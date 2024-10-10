#define FMT_HEADER_ONLY
#include "HighwayGraph.h"
#include "GraphListEntry.h"
#include "HGEdge.h"
#include "HGVertex.h"
#include "PlaceRadius.h"
#include "../Args/Args.h"
#include "../ElapsedTime/ElapsedTime.h"
#include "../HighwaySegment/HighwaySegment.h"
#include "../HighwaySystem/HighwaySystem.h"
#include "../Region/Region.h"
#include "../Route/Route.h"
#include "../TravelerList/TravelerList.h"
#include "../Waypoint/Waypoint.h"
#include "../WaypointQuadtree/WaypointQuadtree.h"
#include "../../templates/contains.cpp"
#include <fmt/format.h>
#include <fstream>
#include <thread>

HighwayGraph::HighwayGraph(WaypointQuadtree &all_waypoints, ElapsedTime &et)
{	unsigned int counter = 0;
	se = 0;
	// create lists of graph points in or colocated with active/preview
	// systems, either singleton at at the front of their colocation lists
	size_t v_idx = 0;
	VInfoVec hi_priority_points, lo_priority_points;
	all_waypoints.graph_points(hi_priority_points, lo_priority_points, v_idx);

	// allocate vertices
	vertices.resize(hi_priority_points.size()+lo_priority_points.size());

	std::cout << et.et() << "Creating unique names and vertices" << std::flush;
      #ifdef threading_enabled
	#define THRLP for (int t=0; t<Args::numthreads; t++) thr[t]
	std::vector<std::thread> thr(Args::numthreads);
	if (Args::mtvertices)
	{	THRLP = std::thread(&HighwayGraph::simplify, this, t, &hi_priority_points, &counter); THRLP.join();
		THRLP = std::thread(&HighwayGraph::simplify, this, t, &lo_priority_points, &counter); THRLP.join();
	} else
      #endif
	{	simplify(0, &hi_priority_points, &counter);
		simplify(0, &lo_priority_points, &counter);
	}
	std::cout << '!' << std::endl;
	cv=tv=vertices.size();

	std::cout << et.et() << "Estimating failsafe edge array size: " << std::flush;
	size_t total_segments = 0;
	for (HighwaySystem& h : HighwaySystem::syslist)
	  if (h.active_or_preview())
	    for (Route& r : h.routes)
	      total_segments += r.segments.size;
	std::cout << total_segments << " total active/preview segments, "
		  << HGVertex::num_hidden << " hidden vertices" << std::endl;

	// create edges
	counter = 0;
	std::cout << et.et() << "Creating edges" << std::flush;
	HGEdge* e = edges.alloc(total_segments + 2*HGVertex::num_hidden);
	for (HighwaySystem& h : HighwaySystem::syslist)
	{	if (!h.active_or_preview()) continue;
		if (counter % 6 == 0) std::cout << '.' << std::flush;
		counter++;
		for (Route& r : h.routes)
		  for (HighwaySegment& s : r.segments)
		    if (&s == s.canonical_edge_segment())
		    { ++se; 
		      new(e++) HGEdge(&s);
		    }
	}
	std::cout << '!' << std::endl;
	ce=te=se;

	// compress edges adjacent to hidden vertices
	counter = 0;
	std::cout << et.et() << "Compressing collapsed edges" << std::flush;
	HGEdge::v_array = vertices.data();
	for (HGVertex& v : vertices)
	{	if (counter % 10000 == 0) std::cout << '.' << std::flush;
		counter++;
		if (!v.visibility)
		{	// <2 edges = HIDDEN_TERMINUS or hidden U-turn
			// >2 edges = HIDDEN_JUNCTION
			// datachecks have been flagged earlier in the program; mark as visible and do not compress
			if (v.edge_count != 2)
			{	v.visibility = 2;
				continue;
			}
			if (!v.incident_edges[0]->segment->same_ap_routes(v.incident_edges[1]->segment))
			{ /*	std::cout << "\nWARNING: segment name mismatch in HGEdge collapse constructor: ";
				std::cout << "edge1 named " << v.incident_edges[0]->segment->segment_name()
					  <<" edge2 named " << v.incident_edges[1]->segment->segment_name() << std::endl;
				std::cout << "  vertex " << *v.unique_name << " unhidden" << std::endl;
				std::cout << "  waypoints:";
				Waypoint* w = v.incident_edges[0]->segment->waypoint2;
				if (w->lat != v.lat || w->lng != v.lng) w = v.incident_edges[0]->segment->waypoint1;
				for (Waypoint* p : *w->colocated) std::cout << ' ' << p->root_at_label();
				std::cout << std::endl; //*/
				v.visibility = 2;
				continue;
			}
			// construct from vertex this time
			--ce; --cv;
			// if edge clinched_by sets mismatch, set visibility to 1
			// (visible in traveled graph; hidden in collapsed graph)
			uint8_t const coll = HGEdge::collapsed, trav = HGEdge::traveled, dual = coll|trav;
			HGEdge* const t_front = v.front(trav);
			HGEdge* const t_back  = v.back (trav);
			if (t_front->segment->clinched_by != t_back->segment->clinched_by)
			{	v.visibility = 1;
				new(e++) HGEdge(&v, coll, v.front(coll), v.back(coll));
				continue;
			}
			if (t_front->format & coll && t_back->format & coll)
				new(e++) HGEdge(&v, dual, t_front, t_back);
			else {	new(e++) HGEdge(&v, coll, v.front(coll), v.back(coll));
				new(e++) HGEdge(&v, trav, t_front, t_back);
			     }
			--te; --tv;
		}
	}
	std::cout << '!' << std::endl;
	edges.size = e - edges.data;

	if (Args::edgecounts)
	{	std::cout << et.et() << "Edge format counts:" << std::endl;
		int fcount[8] = {0,0,0,0,0,0,0,0};
		for (HGEdge& e : edges) fcount[e.format]++;
		int const allocated = total_segments + 2*HGVertex::num_hidden;
		int const live = edges.size-fcount[0];
		double constexpr edge_mb = sizeof(HGEdge)/double(1048576);
		printf("%10i format 0 (temporary, partially collapsed)\n", fcount[0]);
		printf("%10i format 1 (simple)\n", fcount[1]);
		printf("%10i format 2 (collapsed)\n", fcount[2]);
		printf("%10i format 3 (simple + collapsed -- this should always be 0)\n", fcount[3]);
		printf("%10i format 4 (traveled)\n", fcount[4]);
		printf("%10i format 5 (simple + traveled)\n", fcount[5]);
		printf("%10i format 6 (collapsed + traveled)\n", fcount[6]);
		printf("%10i format 7 (simple + collapsed + traveled)\n", fcount[7]);
		printf("-----------------------------------------------------------------\n");
		printf("%10i collapse constructions performed\n", fcount[0]+fcount[2]+fcount[4]+fcount[6]);
		printf("%10i live edges\t\t(%.2f MB)\n", live, live*edge_mb);
		printf("%10li total objects\t(%.2f MB)\n", edges.size, edges.size*edge_mb);
		printf("%10li allocated but unused\t(%.2f MB)\n", allocated-edges.size, (allocated-edges.size)*edge_mb);
		printf("%10i allocated in total\t(%.2f MB)\n", allocated, allocated*edge_mb);
		fflush(stdout);
	}

	Region::it = Region::allregions.begin();
	std::cout << et.et() << "Creating per-region vertex & edge sets." << std::endl;
      #ifdef threading_enabled
	THRLP = std::thread(&Region::ve_thread, &log_mtx, &vertices, &edges);
	THRLP.join();
      #else
	Region::ve_thread(&log_mtx, &vertices, &edges);
      #endif

	HighwaySystem::it = HighwaySystem::syslist.begin();
	std::cout << et.et() << "Creating per-system vertex & edge sets." << std::endl;
      #ifdef threading_enabled
	THRLP = std::thread(&HighwaySystem::ve_thread, &log_mtx, &vertices, &edges);
	THRLP.join();
	#undef THRLP
      #else
	HighwaySystem::ve_thread(&log_mtx, &vertices, &edges);
      #endif

	if (Args::bitsetlogs)
	{	std::cout << et.et() << "Writing TMBitset logs. " << hi_priority_points.size()
			  << " hi_priority_points / " << vertices.size() << " total vertices" << std::endl;
		bitsetlogs(vertices.data() + hi_priority_points.size());
	}

	std::cout << et.et() << "Master graph construction complete. Destroying temporary variables." << std::endl;
} // end ctor

inline std::pair<std::unordered_set<std::string>::iterator,bool> HighwayGraph::vertex_name(std::string& n)
{	set_mtx[n.back()].lock();
	std::pair<std::unordered_set<std::string>::iterator,bool> insertion = vertex_names[n.back()].insert(n);
	set_mtx[n.back()].unlock();
	return insertion;
}

void HighwayGraph::namelog(std::string&& msg)
{	log_mtx.lock();
	waypoint_naming_log.emplace_back(msg);
	log_mtx.unlock();
}

void HighwayGraph::simplify(int t, VInfoVec* points, unsigned int *counter)
{	// create unique names and vertices
	int numthreads = Args::mtvertices ? Args::numthreads : 1;
	auto end = (t+1)*points->size()/numthreads+points->data();
	for (auto vi = t*points->size()/numthreads+points->data(); vi < end; vi++)
	{	// progress indicator
		if (!t)
		{	if (*counter % (10000/numthreads) == 0) std::cout << '.' << std::flush;
			*counter += 1;
		}

		// start with the canonical name and attempt to insert into vertex_names set
		std::string point_name = vi->first->canonical_waypoint_name(this);
		std::pair<std::unordered_set<std::string>::iterator,bool> insertion = vertex_name(point_name);

		// if that's taken, append the region code
		if (!insertion.second)
		{	point_name += "|" + vi->first->route->region->code;
			namelog("Appended region: " + point_name);
			insertion = vertex_name(point_name);
		}

		// if that's taken, see if the simple name is available
		if (!insertion.second)
		{	std::string simple_name = vi->first->simple_waypoint_name();
			insertion = vertex_name(simple_name);
			if (insertion.second)
				namelog("Revert to simple: " + simple_name + " from (taken) " + point_name);
			else do // if we have not yet succeeded, add !'s until we do
			{	point_name += "!";
				namelog("Appended !: " + point_name);
				insertion = vertex_name(point_name);
			} while (!insertion.second);
		}

		// we're good; now set up a vertex
		vertices[vi->second].setup(vi->first, &*(insertion.first));

		// active/preview colocation lists are no longer needed; clear them
		vi->first->ap_coloc.clear();
	}
}

void HighwayGraph::bitsetlogs(HGVertex* hp_end)
{	size_t oldvheap = sizeof(uint64_t) * ceil(double(vertices.size()+1)/(sizeof(uint64_t)*8));
	size_t oldeheap = sizeof(uint64_t) * ceil(double(edges.size+1)/(sizeof(uint64_t)*8));

	size_t final_s, first_c=0, low_pri=0;
	for (size_t i = 0; i < edges.size; ++i)
	{	if (edges[i].format & HGEdge::simple)
			final_s = i;
		else if (!first_c)
			first_c = i;
		else if (&vertices[edges[i].c_idx] == hp_end)
		{	low_pri = i;
			break;
		}
	}
	std::cout << "final_s = " << final_s << ": " << edges[final_s].str() << std::endl;
	std::cout << "first_c = " << first_c << ": " << edges[first_c].str() << std::endl;
	std::cout << "low_pri = " << low_pri << ": " << edges[low_pri].str() << " ~~ " << *hp_end->unique_name << std::endl;
	std::cout << "total_e = " << edges.size << std::endl;

	std::ofstream vramlog(Args::logfilepath+"/tmb-region-vram.csv");
	std::ofstream eramlog(Args::logfilepath+"/tmb-region-eram.csv");
	std::ofstream vgaplog(Args::logfilepath+"/tmb-region-vgap.csv");
	std::ofstream egaplog(Args::logfilepath+"/tmb-region-egap.csv");
	using x = TMBitset<void*,uint64_t>*;
	auto ramlogline=[](x tmb, std::string& code, std::ofstream& log, size_t old_heap)
	{	log << code << ';' << tmb->count() << ';' << old_heap << ';'<< tmb->vec_cap() 
		    << ';' << tmb->vec_size() << ';' << tmb->heap() << std::endl;
	};
	auto vgaplogline=[&](TMBitset<HGVertex*,uint64_t>& tmb, std::string& code, HGVertex* start)
	{	HGVertex *lo_v, *hi_v, *lo_g, *hi_g, *prev;
		lo_v = *tmb.begin();
		prev = lo_v;
		size_t gap = 0;
		for (auto v : tmb)
		{	hi_v = v;
			if (gap < v-prev)
			{	gap = v-prev;
				hi_g = v;
				lo_g = prev;
			}
			prev = v;
		}
		vgaplog << code
			<< ';' << lo_v-start << ';' << *lo_v->unique_name << ';' << gap
			<< ';' << lo_g-start << ';' << *lo_g->unique_name << ';' << (lo_g < hp_end ? "hi" : "lo")
			<< ';' << hi_g-start << ';' << *hi_g->unique_name << ';' << (hi_g < hp_end ? "hi" : "lo")
			<< ';' << hi_v-start << ';' << *hi_v->unique_name << std::endl;
	};
	auto egaplogline=[&](TMBitset<HGEdge*,uint64_t>& tmb, std::string& code, HGEdge* start)
	{	HGEdge *lo_e, *hi_e, *lo_g1, *hi_g1, *lo_g2, *hi_g2, *prev;
		lo_e = *tmb.begin();
		prev = lo_e;
		size_t gap_1=0, gap_2=0;
		for (auto e : tmb)
		{	hi_e = e;
			if (gap_2 < e-prev)
			  if (gap_1 < e-prev)
			      {	gap_2 = gap_1;	hi_g2 = hi_g1;	lo_g2 = lo_g1;
				gap_1 = e-prev;	hi_g1 = e;	lo_g1 = prev;
			      }
			  else{	gap_2 = e-prev;	hi_g2 = e;	lo_g2 = prev;
			      }
			prev = e;
		}
		auto info = [&](size_t gap, HGEdge* e){return gap ? e->str() : "NO GAP";};
		auto pri  = [&](size_t gap, HGEdge* e)
		{	return gap ? e->format & HGEdge::simple ? "s" : &vertices[e->c_idx] < hp_end ? "hi" : "lo" : "NOPE";
		};
		egaplog << code << std::flush
			<< ';' << lo_e -start << ';' <<    lo_e->str()    << ';' << gap_1
			<< ';' << lo_g1-start << ';' << info(gap_1,lo_g1) << ';' << pri(gap_1,lo_g1)
			<< ';' << hi_g1-start << ';' << info(gap_1,hi_g1) << ';' << pri(gap_1,hi_g1) << ';' << gap_2
			<< ';' << lo_g2-start << ';' << info(gap_2,lo_g2) << ';' << pri(gap_2,lo_g2)
			<< ';' << hi_g2-start << ';' << info(gap_2,hi_g2) << ';' << pri(gap_2,hi_g2)
			<< ';' << hi_e -start << ';' <<    hi_e->str()    << std::endl;
	};

	vramlog << "Region" << ";Count;OldHeap;VecCap;VecSize;NewHeap\n";
	eramlog << "Region" << ";Count;OldHeap;VecCap;VecSize;NewHeap\n";
	vgaplog << "Region" << ";LoIndex;LoName;gap;Beg;BegPt;BegPri;End;EndPt;EndPri;HiIndex;HiName\n";
	egaplog << "Region" << ";LoIndex;LoInfo"
			    << ";gap1;Beg1;BegInfo1;BegPri1;End1;EndInfo1;EndPri1"
			    << ";gap2;Beg2;BegInfo2;BegPri2;End2;EndInfo2;EndPri2;HiIndex;HiInfo\n";
	for (Region& rg : Region::allregions)
	{ if (!rg.vertices.is_null_set())
	  {	ramlogline(x(&rg.vertices), rg.code, vramlog, oldvheap);
		vgaplogline ( rg.vertices,  rg.code, vertices.data() );
	  }
	  if (!rg.edges.is_null_set())
	  {	   ramlogline(x(&rg.edges), rg.code, eramlog, oldeheap);
		   egaplogline ( rg.edges,  rg.code, edges.data );
	  }
	}

	vramlog.close(); vramlog.open(Args::logfilepath+"/tmb-system-vram.csv");
	eramlog.close(); eramlog.open(Args::logfilepath+"/tmb-system-eram.csv");
	vgaplog.close(); vgaplog.open(Args::logfilepath+"/tmb-system-vgap.csv");
	egaplog.close(); egaplog.open(Args::logfilepath+"/tmb-system-egap.csv");

	vramlog << "System" << ";Count;OldHeap;VecCap;VecSize;NewHeap\n";
	eramlog << "System" << ";Count;OldHeap;VecCap;VecSize;NewHeap\n";
	vgaplog << "System" << ";LoIndex;LoName;gap;Beg;BegPt;BegPri;End;EndPt;EndPri;HiIndex;HiName\n";
	egaplog << "System" << ";LoIndex;LoInfo"
			    << ";gap1;Beg1;BegInfo1;BegPri1;End1;EndInfo1;EndPri1"
			    << ";gap2;Beg2;BegInfo2;BegPri2;End2;EndInfo2;EndPri2;HiIndex;HiInfo\n";
	for (HighwaySystem& h : HighwaySystem::syslist)
	{ if (!h.vertices.is_null_set())
	  {	ramlogline(x(&h.vertices), h.systemname, vramlog, oldvheap);
		vgaplogline ( h.vertices,  h.systemname, vertices.data() );
	  }
	  if (!h.edges.is_null_set())
	  {	   ramlogline(x(&h.edges), h.systemname, eramlog, oldeheap);
		   egaplogline ( h.edges,  h.systemname, edges.data );
	  }
	}
}

// write the entire set of highway data in .tmg format.
// The first line is a header specifying the format and version number,
// The second line specifies the number of waypoints, w, the number of connections, c,
//     and for traveled graphs only, the number of travelers.
// Then, w lines describing waypoints (label, latitude, longitude).
// Then, c lines describing connections (endpoint 1 number, endpoint 2 number, route label),
//     followed on traveled graphs only by a hexadecimal code encoding travelers on that segment,
//     followed on both collapsed & traveled graphs by a list of latitude & longitude values
//     for intermediate "shaping points" along the edge, ordered from endpoint 1 to endpoint 2.
//
void HighwayGraph::write_master_graphs_tmg()
{	std::ofstream simplefile(Args::graphfilepath + "/tm-master-simple.tmg");
	std::ofstream collapfile(Args::graphfilepath + "/tm-master.tmg");
	std::ofstream travelfile(Args::graphfilepath + "/tm-master-traveled.tmg");
	simplefile << "TMG 1.0 simple\n";
	collapfile << "TMG 1.0 collapsed\n";
	travelfile << "TMG 2.0 traveled\n";
	simplefile << vertices.size() << ' ' << se << '\n';
	collapfile << cv << ' ' << ce << '\n';
	travelfile << tv << ' ' << te << ' ' << TravelerList::allusers.size << '\n';

	// write vertices
	unsigned int sv = 0;
	unsigned int cv = 0;
	unsigned int tv = 0;
	char fstr[57];
	for (HGVertex& v : vertices)
	{	*fmt::format_to(fstr, " {:.15} {:.15}", v.lat, v.lng) = 0;
		switch (v.visibility) // fall-thru is a Good Thing!
		{ case 2:  collapfile << *(v.unique_name) << fstr << '\n'; v.c_vertex_num[0] = cv++;
		  case 1:  travelfile << *(v.unique_name) << fstr << '\n'; v.t_vertex_num[0] = tv++;
		  default: simplefile << *(v.unique_name) << fstr << '\n'; v.s_vertex_num[0] = sv++;
		}
	}

	// allocate clinched_by code
	size_t nibbles = ceil(double(TravelerList::allusers.size)/4);
	char* cbycode = new char[nibbles+1];
			// deleted after writing edges
	cbycode[nibbles] = 0;

	// write edges
	//TODO: multiple functions performing the same instructions for multiple files?
	for (HGEdge *e = edges.begin(), *end = edges.end(); e != end; ++e)
	{ if (e->format & HGEdge::collapsed)
		e->collapsed_tmg_line(collapfile, fstr, 0, 0);
	  if (e->format & HGEdge::traveled)
	  {	for (char*n=cbycode; n<cbycode+nibbles; ++n) *n = '0';
		e->traveled_tmg_line(travelfile, fstr, 0, 0, TravelerList::allusers.size, cbycode);
	  }
	  if (e->format & HGEdge::simple)
	  {	simplefile << e->vertex1->s_vertex_num[0] << ' '
			   << e->vertex2->s_vertex_num[0] << ' ';
		e->segment->write_label(simplefile, 0);
		simplefile << '\n';
	  }
	}
	delete[] cbycode;

	// traveler names
	for (TravelerList& t : TravelerList::allusers)
		travelfile << t.traveler_name << ' ';
	travelfile << '\n';
	simplefile.close();
	collapfile.close();
	travelfile.close();
	GraphListEntry* g = GraphListEntry::entries.data();
	g[0].vertices = vertices.size(); g[0].edges = se; g[0].travelers = 0;
	g[1].vertices = cv;		 g[1].edges = ce; g[1].travelers = 0;
	g[2].vertices = tv;		 g[2].edges = te; g[2].travelers = TravelerList::allusers.size;
}

// write a subset of the data,
// in simple, collapsed and traveled formats,
// restricted by regions in the list if given,
// by systems in the list if given,
// or to within a given area if placeradius is given
void HighwayGraph::write_subgraphs_tmg
(	size_t graphnum, unsigned int threadnum, WaypointQuadtree *qt, ElapsedTime *et, std::mutex *term
)
{	unsigned int cv_count = 0, sv_count = 0, tv_count = 0;
	unsigned int ce_count = 0, se_count = 0, te_count = 0;
	GraphListEntry* g = GraphListEntry::entries.data()+graphnum;
	std::ofstream simplefile(Args::graphfilepath+'/'+g -> filename());
	std::ofstream collapfile(Args::graphfilepath+'/'+g[1].filename());
	std::ofstream travelfile(Args::graphfilepath+'/'+g[2].filename());
	TMBitset<HGVertex*, uint64_t> mv; // vertices matching all criteria
	TMBitset<HGEdge*,   uint64_t> me; //    edges matching all criteria
	std::vector<TravelerList*> traveler_lists;
	TMBitset<TravelerList*, uint32_t> traveler_set(TravelerList::allusers.data, TravelerList::allusers.size);
	#include "get_subgraph_data.cpp"
	// assign traveler numbers
	unsigned int travnum = 0;
	for (TravelerList *t : traveler_set)
	{	t->traveler_num[threadnum] = travnum++;
		traveler_lists.push_back(t);
	}
      #ifdef threading_enabled
	term->lock();
      #endif
	if (g->cat != g[-1].cat)
		std::cout << '\n' << et->et() << "Writing " << g->category() << " graphs.\n";
	std::cout << g->tag()
		  << '(' << sv_count << ',' << se_count << ") "
		  << '(' << cv_count << ',' << ce_count << ") "
		  << '(' << tv_count << ',' << te_count << ") " << std::flush;
      #ifdef threading_enabled
	term->unlock();
      #endif
	simplefile << "TMG 1.0 simple\n";
	collapfile << "TMG 1.0 collapsed\n";
	travelfile << "TMG 2.0 traveled\n";
	simplefile << sv_count << ' ' << se_count << '\n';
	collapfile << cv_count << ' ' << ce_count << '\n';
	travelfile << tv_count << ' ' << te_count << ' ' << travnum << '\n';

	// write vertices
	unsigned int sv = 0;
	unsigned int cv = 0;
	unsigned int tv = 0;
	char fstr[57];
	for (HGVertex *v : mv)
	{	*fmt::format_to(fstr, " {:.15} {:.15}", v->lat, v->lng) = 0;
		switch(v->visibility) // fall-thru is a Good Thing!
		{ case 2:  collapfile << *(v->unique_name) << fstr << '\n'; v->c_vertex_num[threadnum] = cv++;
		  case 1:  travelfile << *(v->unique_name) << fstr << '\n'; v->t_vertex_num[threadnum] = tv++;
		  default: simplefile << *(v->unique_name) << fstr << '\n'; v->s_vertex_num[threadnum] = sv++;
		}
	}

	// allocate clinched_by code
	size_t nibbles = ceil(double(travnum)/4);
	char* cbycode = new char[nibbles+1];
			// deleted after writing edges
	cbycode[nibbles] = 0;

	// write edges
	for (HGEdge *e : me) //TODO: multiple functions performing the same instructions for multiple files?
	{ if (e->format & HGEdge::simple)
	  {	simplefile << e->vertex1->s_vertex_num[threadnum] << ' '
			   << e->vertex2->s_vertex_num[threadnum] << ' ';
		e->segment->write_label(simplefile, g->systems);
		simplefile << '\n';
	  }
	  if (e->format & HGEdge::collapsed)
		e->collapsed_tmg_line(collapfile, fstr, threadnum, g->systems);
	  if (e->format & HGEdge::traveled)
	  {	for (char*n=cbycode; n<cbycode+nibbles; ++n) *n = '0';
		e->traveled_tmg_line (travelfile, fstr, threadnum, g->systems, travnum, cbycode);
	  }
	}
	delete[] cbycode;

	// traveler names
	for (TravelerList *t : traveler_lists)
		travelfile << t->traveler_name << ' ';
	travelfile << '\n';
	simplefile.close();
	collapfile.close();
	travelfile.close();
	if (g->regions) delete g->regions;
	if (g->systems) delete g->systems;
	if (g->placeradius) delete g->placeradius;

	g -> vertices = sv_count; g -> edges = se_count; g -> travelers = 0;
	g[1].vertices = cv_count; g[1].edges = ce_count; g[1].travelers = 0;
	g[2].vertices = tv_count; g[2].edges = te_count; g[2].travelers = travnum;
}
