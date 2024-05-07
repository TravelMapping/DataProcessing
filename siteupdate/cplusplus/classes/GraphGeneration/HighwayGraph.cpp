#include "HighwayGraph.h"
#include "GraphListEntry.h"
#include "HGEdge.h"
#include "HGVertex.h"
#include "PlaceRadius.h"
#include "../Args/Args.h"
#include "../Datacheck/Datacheck.h"
#include "../ElapsedTime/ElapsedTime.h"
#include "../HighwaySegment/HighwaySegment.h"
#include "../HighwaySystem/HighwaySystem.h"
#include "../Region/Region.h"
#include "../Route/Route.h"
#include "../TravelerList/TravelerList.h"
#include "../Waypoint/Waypoint.h"
#include "../WaypointQuadtree/WaypointQuadtree.h"
#include "../../templates/contains.cpp"
#include <fstream>
#include <thread>

HighwayGraph::HighwayGraph(WaypointQuadtree &all_waypoints, ElapsedTime &et)
{	unsigned int counter = 0;
	se = 0;
	// create lists of graph points in or colocated with active/preview
	// systems, either singleton at at the front of their colocation lists
	std::vector<Waypoint*> hi_priority_points, lo_priority_points;
	all_waypoints.graph_points(hi_priority_points, lo_priority_points);

	// allocate vertices, and a bit field to track their inclusion in subgraphs
	vertices.resize(hi_priority_points.size()+lo_priority_points.size());
	vbytes = ceil(double(vertices.size())/8);
	vbits = new unsigned char[vbytes*Args::numthreads];
		// deleted by HighwayGraph::clear
	for (size_t i = 0; i < vbytes*Args::numthreads; ++i) vbits[i] = 0;

	std::cout << et.et() << "Creating unique names and vertices" << std::flush;
      #ifdef threading_enabled
	if (Args::mtvertices)
	{	std::vector<std::thread> thr(Args::numthreads);
		#define THRLP for (int t=0; t<Args::numthreads; t++) thr[t]
		THRLP = std::thread(&HighwayGraph::simplify, this, t, &hi_priority_points, &counter, 0);
		THRLP.join();
		THRLP = std::thread(&HighwayGraph::simplify, this, t, &lo_priority_points, &counter, hi_priority_points.size());
		THRLP.join();
		#undef THRLP
	} else
      #endif
	{	simplify(0, &hi_priority_points, &counter, 0);
		simplify(0, &lo_priority_points, &counter, hi_priority_points.size());
	}
	std::cout << '!' << std::endl;
	hi_priority_points.insert(hi_priority_points.end(), lo_priority_points.begin(), lo_priority_points.end());
	lo_priority_points.clear();
	cv=tv=vertices.size();

	// create edges
	counter = 0;
	std::cout << et.et() << "Creating edges" << std::flush;
	for (HighwaySystem& h : HighwaySystem::syslist)
	{	if (!h.active_or_preview()) continue;
		if (counter % 6 == 0) std::cout << '.' << std::flush;
		counter++;
		for (Route& r : h.routes)
		  for (HighwaySegment& s : r.segments)
		    if (&s == s.canonical_edge_segment())
		    { ++se; 
		      new HGEdge(&s);
		      // deleted by HGEdge::detach via ~HGVertex via HighwayGraph::clear
		    }
	}
	std::cout << '!' << std::endl;
	ce=te=se;

	// compress edges adjacent to hidden vertices
	counter = 0;
	std::cout << et.et() << "Compressing collapsed edges" << std::flush;
	for (Waypoint* w : hi_priority_points)
	{	if (counter % 10000 == 0) std::cout << '.' << std::flush;
		counter++;
		if (!w->vertex->visibility)
		{	// <2 edges = HIDDEN_TERMINUS
			// >2 edges = HIDDEN_JUNCTION
			// datachecks have been flagged earlier in the program; mark as visible and do not compress
			if (w->vertex->incident_c_edges.size() != 2)
			{	w->vertex->visibility = 2;
				continue;
			}
			// construct from vertex this time
			--ce; --cv;
			// if edge clinched_by sets mismatch, set visibility to 1
			// (visible in traveled graph; hidden in collapsed graph)
			if (w->vertex->incident_t_edges.front()->segment->clinched_by
			 != w->vertex->incident_t_edges.back()->segment->clinched_by)
			{	w->vertex->visibility = 1;
				new HGEdge(w->vertex, HGEdge::collapsed);
				continue;
			}
			if   (	 (w->vertex->incident_c_edges.front() == w->vertex->incident_t_edges.front()
			       && w->vertex->incident_c_edges.back()  == w->vertex->incident_t_edges.back())
			      || (w->vertex->incident_c_edges.front() == w->vertex->incident_t_edges.back()
			       && w->vertex->incident_c_edges.back()  == w->vertex->incident_t_edges.front())
			     )	new HGEdge(w->vertex, HGEdge::collapsed | HGEdge::traveled);
			else {	new HGEdge(w->vertex, HGEdge::collapsed);
				new HGEdge(w->vertex, HGEdge::traveled);
				// Final collapsed edges are deleted by HGEdge::detach via ~HGVertex via HighwayGraph::clear.
				// Partially collapsed edges created during the compression process
				// are deleted by HGEdge::detach upon detachment from all graphs.
			     }
			--te; --tv;
		}
	}
	std::cout << '!' << std::endl;

	std::cout << et.et() << "Master graph construction complete. Destroying temporary variables." << std::endl;
} // end ctor

void HighwayGraph::clear()
{	delete[] vbits;
	for (int i=0;i<256;i++) vertex_names[i].clear();
	waypoint_naming_log.clear();
	vertices.clear();
}

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

void HighwayGraph::simplify(int t, std::vector<Waypoint*>* points, unsigned int *counter, const size_t offset)
{	// create unique names and vertices
	int numthreads = Args::mtvertices ? Args::numthreads : 1;
	int e = (t+1)*points->size()/numthreads;
	for (int w = t*points->size()/numthreads; w < e; w++)
	{	// progress indicator
		if (!t)
		{	if (*counter % (10000/numthreads) == 0) std::cout << '.' << std::flush;
			*counter += 1;
		}

		// start with the canonical name and attempt to insert into vertex_names set
		std::string point_name = (*points)[w]->canonical_waypoint_name(this);
		std::pair<std::unordered_set<std::string>::iterator,bool> insertion = vertex_name(point_name);

		// if that's taken, append the region code
		if (!insertion.second)
		{	point_name += "|" + (*points)[w]->route->region->code;
			namelog("Appended region: " + point_name);
			insertion = vertex_name(point_name);
		}

		// if that's taken, see if the simple name is available
		if (!insertion.second)
		{	std::string simple_name = (*points)[w]->simple_waypoint_name();
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
		vertices[offset+w].setup((*points)[w], &*(insertion.first));

		// active/preview colocation lists are no longer needed; clear them
		(*points)[w]->ap_coloc.clear();
	}
}

// HGVertex subgraph membership
bool HighwayGraph::subgraph_contains(HGVertex* v, const int threadnum)
{	size_t index = v-vertices.data();
	return vbits[threadnum*vbytes+index/8] & 1 << index%8;
}
void HighwayGraph::add_to_subgraph(HGVertex* v, const int threadnum)
{	size_t index = v-vertices.data();
	vbits[threadnum*vbytes+index/8] |= 1 << index%8;
}
void HighwayGraph::clear_vbit(HGVertex* v, const int threadnum)
{	size_t index = v-vertices.data();
	vbits[threadnum*vbytes+index/8] &= ~(1 << index%8);
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
	{	sprintf(fstr, " %.15g %.15g", v.lat, v.lng);
		switch (v.visibility) // fall-thru is a Good Thing!
		{ case 2:  collapfile << *(v.unique_name) << fstr << '\n'; v.c_vertex_num[0] = cv++;
		  case 1:  travelfile << *(v.unique_name) << fstr << '\n'; v.t_vertex_num[0] = tv++;
		  default: simplefile << *(v.unique_name) << fstr << '\n'; v.s_vertex_num[0] = sv++;
		}
	}
	// now edges, only write if not already written
	size_t nibbles = ceil(double(TravelerList::allusers.size)/4);
	char* cbycode = new char[nibbles+1];
			// deleted after writing edges
	cbycode[nibbles] = 0;
	for (HGVertex& v : vertices)
	  switch (v.visibility) // fall-thru is a Good Thing!
	  { case 2:	for (HGEdge *e : v.incident_c_edges)
			  if (!(e->written[0] &  HGEdge::collapsed))
			  {	e->written[0] |= HGEdge::collapsed;
				e->collapsed_tmg_line(collapfile, fstr, 0, 0);
			  }
	    case 1:	for (HGEdge *e : v.incident_t_edges)
			  if (!(e->written[0] &  HGEdge::traveled))
			  {	e->written[0] |= HGEdge::traveled;
				for (char*n=cbycode; n<cbycode+nibbles; ++n) *n = '0';
				e->traveled_tmg_line(travelfile, fstr, 0, 0, TravelerList::allusers.size, cbycode);
			  }
	    default:	for (HGEdge *e : v.incident_s_edges)
			  if (!(e->written[0] &  HGEdge::simple))
			  {	e->written[0] |= HGEdge::simple;
				simplefile << e->vertex1->s_vertex_num[0] << ' '
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
{	unsigned int cv_count = 0;
	unsigned int tv_count = 0;
	GraphListEntry* g = GraphListEntry::entries.data()+graphnum;
	std::ofstream simplefile(Args::graphfilepath+'/'+g -> filename());
	std::ofstream collapfile(Args::graphfilepath+'/'+g[1].filename());
	std::ofstream travelfile(Args::graphfilepath+'/'+g[2].filename());
	std::vector<HGVertex*> mv;		// vertices matching all criteria
	std::vector<HGEdge*> mse, mce, mte;	// matching simple/collapsed/traveled edges
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
		  << '(' << mv.size() << ',' << mse.size() << ") "
		  << '(' << cv_count << ',' << mce.size() << ") "
		  << '(' << tv_count << ',' << mte.size() << ") " << std::flush;
      #ifdef threading_enabled
	term->unlock();
      #endif
	simplefile << "TMG 1.0 simple\n";
	collapfile << "TMG 1.0 collapsed\n";
	travelfile << "TMG 2.0 traveled\n";
	simplefile << mv.size() << ' ' << mse.size() << '\n';
	collapfile << cv_count << ' ' << mce.size() << '\n';
	travelfile << tv_count << ' ' << mte.size() << ' ' << travnum << '\n';

	// write vertices
	unsigned int sv = 0;
	unsigned int cv = 0;
	unsigned int tv = 0;
	char fstr[57];
	for (HGVertex *v : mv)
	{	sprintf(fstr, " %.15g %.15g", v->lat, v->lng);
		switch(v->visibility) // fall-thru is a Good Thing!
		{ case 2:  collapfile << *(v->unique_name) << fstr << '\n'; v->c_vertex_num[threadnum] = cv++;
		  case 1:  travelfile << *(v->unique_name) << fstr << '\n'; v->t_vertex_num[threadnum] = tv++;
		  default: simplefile << *(v->unique_name) << fstr << '\n'; v->s_vertex_num[threadnum] = sv++;
		}
	}
	// write edges
	for (HGEdge *e : mse)
	{	simplefile << e->vertex1->s_vertex_num[threadnum] << ' '
			   << e->vertex2->s_vertex_num[threadnum] << ' ';
		e->segment->write_label(simplefile, g->systems);
		simplefile << '\n';
	}
	for (HGEdge *e : mce)
		e->collapsed_tmg_line(collapfile, fstr, threadnum, g->systems);
	size_t nibbles = ceil(double(travnum)/4);
	char* cbycode = new char[nibbles+1];
			// deleted after writing edges
	cbycode[nibbles] = 0;
	for (HGEdge *e : mte)
	{	for (char*n=cbycode; n<cbycode+nibbles; ++n) *n = '0';
		e->traveled_tmg_line (travelfile, fstr, threadnum, g->systems, travnum, cbycode);
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

	g -> vertices = mv.size(); g -> edges = mse.size(); g -> travelers = 0;
	g[1].vertices = cv_count;  g[1].edges = mce.size(); g[1].travelers = 0;
	g[2].vertices = tv_count;  g[2].edges = mte.size(); g[2].travelers = travnum;
}
