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
#include "../../templates/set_intersection.cpp"
#include <fstream>
#include <thread>

HighwayGraph::HighwayGraph(WaypointQuadtree &all_waypoints, ElapsedTime &et)
{	unsigned int counter = 0;
	// create lists of graph points in or colocated with active/preview
	// systems, either singleton at at the front of their colocation lists
	std::vector<Waypoint*> hi_priority_points, lo_priority_points;
	all_waypoints.graph_points(hi_priority_points, lo_priority_points);
	std::cout << et.et() << "Creating unique names and vertices" << std::flush;
	// temporary containers to store vertices during threaded construction
	std::vector<HGVertex*>* sublists = new std::vector<HGVertex*>[Args::numthreads];
					   // deleted after copying into vertices
	#define THRLP for (int t=0; t<Args::numthreads; t++)
      #ifdef threading_enabled
	if (Args::mtvertices)
	{	std::vector<std::thread> thr(Args::numthreads);
		THRLP thr[t] = std::thread(&HighwayGraph::simplify, this, t, &hi_priority_points, &counter, sublists+t);
		THRLP thr[t].join();
		THRLP thr[t] = std::thread(&HighwayGraph::simplify, this, t, &lo_priority_points, &counter, sublists+t);
		THRLP thr[t].join();
	} else
      #endif
	{	simplify(0, &hi_priority_points, &counter, sublists);
		simplify(0, &lo_priority_points, &counter, sublists);
	}
	std::cout << '!' << std::endl;
	THRLP vertices.insert(vertices.end(), sublists[t].begin(), sublists[t].end());
	#undef THRLP
	delete[] sublists;
	hi_priority_points.insert(hi_priority_points.end(), lo_priority_points.begin(), lo_priority_points.end());
	lo_priority_points.clear();

	// create edges
	counter = 0;
	std::cout << et.et() << "Creating edges" << std::flush;
	for (HighwaySystem *h : HighwaySystem::syslist)
	{	if (!h->active_or_preview()) continue;
		if (counter % 6 == 0) std::cout << '.' << std::flush;
		counter++;
		for (Route *r : h->route_list)
		  for (HighwaySegment *s : r->segment_list)
		    if (!s->concurrent || s == s->concurrent->front())
		      new HGEdge(s, this, Args::numthreads);
		      // deleted by ~HGVertex, called by HighwayGraph::clear
	}
	std::cout << '!' << std::endl;

	// compress edges adjacent to hidden vertices
	counter = 0;
	std::cout << et.et() << "Compressing collapsed edges" << std::flush;
	for (Waypoint* w : hi_priority_points)
	{	if (counter % 10000 == 0) std::cout << '.' << std::flush;
		counter++;
		if (!w->vertex->visibility)
		{	// cases with only one edge are flagged as HIDDEN_TERMINUS
			if (w->vertex->incident_c_edges.size() < 2)
			{	w->vertex->visibility = 2;
				continue;
			}
			// if >2 edges, flag HIDDEN_JUNCTION, mark as visible, and do not compress
			if (w->vertex->incident_c_edges.size() > 2)
			{	Datacheck::add(w->colocated->front()->route,
					       w->colocated->front()->label,
					       "", "", "HIDDEN_JUNCTION", std::to_string(w->vertex->incident_c_edges.size()));
				w->vertex->visibility = 2;
				continue;
			}
			// if edge clinched_by sets mismatch, set visibility to 1
			// (visible in traveled graph; hidden in collapsed graph)
			// first, the easy check, for whether set sizes mismatch
			if (w->vertex->incident_t_edges.front()->segment->clinched_by.size()
			 != w->vertex->incident_t_edges.back()->segment->clinched_by.size())
				w->vertex->visibility = 1;
			// next, compare clinched_by sets; look for any element in the 1st not in the 2nd
			else for (TravelerList *t : w->vertex->incident_t_edges.front()->segment->clinched_by)
				if (w->vertex->incident_t_edges.back()->segment->clinched_by.find(t)
				 == w->vertex->incident_t_edges.back()->segment->clinched_by.end())
				{	w->vertex->visibility = 1;
					break;
				}
			// construct from vertex this time
			if (w->vertex->visibility == 1)
				new HGEdge(w->vertex, HGEdge::collapsed, Args::numthreads);
			else if ((w->vertex->incident_c_edges.front() == w->vertex->incident_t_edges.front()
			       && w->vertex->incident_c_edges.back()  == w->vertex->incident_t_edges.back())
			      || (w->vertex->incident_c_edges.front() == w->vertex->incident_t_edges.back()
			       && w->vertex->incident_c_edges.back()  == w->vertex->incident_t_edges.front()))
				new HGEdge(w->vertex, HGEdge::collapsed | HGEdge::traveled, Args::numthreads);
			else {	new HGEdge(w->vertex, HGEdge::collapsed, Args::numthreads);
				new HGEdge(w->vertex, HGEdge::traveled, Args::numthreads);
				// Final collapsed edges are deleted by ~HGVertex, called by HighwayGraph::clear.
				// Partially collapsed edges created during the compression process are deleted
				// upon detachment from all graphs.
			     }
		}
	}
	std::cout << '!' << std::endl;
} // end ctor

void HighwayGraph::clear()
{	for (HGVertex *v : vertices) delete v;
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

void HighwayGraph::simplify(int t, std::vector<Waypoint*>* points, unsigned int *counter, std::vector<HGVertex*>* sublist)
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

		// we're good; now construct a vertex
		sublist->emplace_back(new HGVertex((*points)[w], &*(insertion.first), Args::numthreads));
				      // deleted by HighwayGraph::clear

		// active/preview colocation lists are no longer needed; clear them
		(*points)[w]->ap_coloc.clear();
	}
}

inline void HighwayGraph::matching_vertices_and_edges
(	GraphListEntry &g, WaypointQuadtree *qt,
	std::list<TravelerList*> &traveler_lists,
	std::unordered_set<HGVertex*> &mvset,	// final set of vertices matching all criteria
	std::list<HGEdge*> &mse,		// matching    simple edges
	std::list<HGEdge*> &mce,		// matching collapsed edges
	std::list<HGEdge*> &mte,		// matching  traveled edges
	int threadnum, unsigned int &cv_count, unsigned int &tv_count
)
{	// Find a set of vertices from the graph, optionally
	// restricted by region or system or placeradius area.
	cv_count = 0;
	tv_count = 0;
	std::unordered_set<HGVertex*> rvset;	// union of all sets in regions
	std::unordered_set<HGVertex*> svset;	// union of all sets in systems
	std::unordered_set<HGVertex*> pvset;	// set of vertices within placeradius

	if (g.regions) for (Region *r : *g.regions)
		rvset.insert(r->vertices.begin(), r->vertices.end());
	if (g.systems) for (HighwaySystem *h : *g.systems)
		svset.insert(h->vertices.begin(), h->vertices.end());
	if (g.placeradius)
		pvset = g.placeradius->vertices(qt, this);

	// determine which vertices are within our region(s) and/or system(s)
	if (g.regions)
	{	mvset = rvset;
		if (g.placeradius)	mvset = mvset & pvset;
		if (g.systems)		mvset = mvset & svset;
	}
	else if (g.systems)
	{	mvset = svset;
		if (g.placeradius)	mvset = mvset & pvset;
	}
	else if (g.placeradius)
		mvset = pvset;
	else	// no restrictions via region, system, or placeradius, so include everything
		mvset.insert(vertices.begin(), vertices.end());

	// initialize *_written booleans
	for (HGVertex *v : mvset)
	{	for (HGEdge* e : v->incident_s_edges) e->s_written[threadnum] = 0;
		for (HGEdge* e : v->incident_c_edges) e->c_written[threadnum] = 0;
		for (HGEdge* e : v->incident_t_edges) e->t_written[threadnum] = 0;
	}

	// Compute sets of edges for subgraphs, optionally
	// restricted by region or system or placeradius.
	// Keep a count of collapsed & traveled vertices as we go.
	for (HGVertex *v : mvset)
	{	for (HGEdge *e : v->incident_s_edges)
		  if ((!g.placeradius || g.placeradius->contains_edge(e)) && !e->s_written[threadnum])
		    if (!g.regions || contains(*g.regions, e->segment->route->region))
		    {	bool system_match = !g.systems;
			if (!system_match)
			  for (std::pair<std::string, HighwaySystem*> &rs : e->route_names_and_systems)
			    if (contains(*g.systems, rs.second))
			    {	system_match = 1;
				break;
			    }
			if (system_match)
			{	mse.push_back(e);
				e->s_written[threadnum] = 1;
			}
		    }
		if (v->visibility < 1) continue;
		tv_count++;
		for (HGEdge *e : v->incident_t_edges)
		  if ((!g.placeradius || g.placeradius->contains_edge(e)) && !e->t_written[threadnum])
		    if (!g.regions || contains(*g.regions, e->segment->route->region))
		    {	bool system_match = !g.systems;
			if (!system_match)
			  for (std::pair<std::string, HighwaySystem*> &rs : e->route_names_and_systems)
			    if (contains(*g.systems, rs.second))
			    {	system_match = 1;
				break;
			    }
			if (system_match)
			{	mte.push_back(e);
				e->t_written[threadnum] = 1;
				for (TravelerList *t : e->segment->clinched_by)
				  if (!t->in_subgraph[threadnum])
				  {	traveler_lists.push_back(t);
					t->in_subgraph[threadnum] = 1;
				  }
			}
		    }
		if (v->visibility < 2) continue;
		cv_count++;
		for (HGEdge *e : v->incident_c_edges)
		  if ((!g.placeradius || g.placeradius->contains_edge(e)) && !e->c_written[threadnum])
		    if (!g.regions || contains(*g.regions, e->segment->route->region))
		    {	bool system_match = !g.systems;
			if (!system_match)
			  for (std::pair<std::string, HighwaySystem*> &rs : e->route_names_and_systems)
			    if (contains(*g.systems, rs.second))
			    {	system_match = 1;
				break;
			    }
			if (system_match)
			{	mce.push_back(e);
				e->c_written[threadnum] = 1;
			}
		    }
	}
	for (TravelerList* t : traveler_lists) t->in_subgraph[threadnum] = 0;
	traveler_lists.sort(sort_travelers_by_name);
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
	std::ofstream collapfile(Args::graphfilepath + "/tm-master-collapsed.tmg");
	std::ofstream travelfile(Args::graphfilepath + "/tm-master-traveled.tmg");
	simplefile << "TMG 1.0 simple\n";
	collapfile << "TMG 1.0 collapsed\n";
	travelfile << "TMG 2.0 traveled\n";
	simplefile << GraphListEntry::entries[0].vertices << ' ' << GraphListEntry::entries[0].edges << '\n';
	collapfile << GraphListEntry::entries[1].vertices << ' ' << GraphListEntry::entries[1].edges << '\n';
	travelfile << GraphListEntry::entries[2].vertices << ' ' << GraphListEntry::entries[2].edges << ' '
		   << TravelerList::allusers.size() << '\n';

	// write vertices
	unsigned int sv = 0;
	unsigned int cv = 0;
	unsigned int tv = 0;
	for (HGVertex* v : vertices)
	{	char fstr[57];
		sprintf(fstr, " %.15g %.15g", v->lat, v->lng);
		// all vertices for simple graph
		simplefile << *(v->unique_name) << fstr << '\n';
		v->s_vertex_num[0] = sv;
		sv++;
		// visible vertices...
		if (v->visibility >= 1)
		{	// for traveled graph,
			travelfile << *(v->unique_name) << fstr << '\n';
			v->t_vertex_num[0] = tv;
			tv++;
			if (v->visibility == 2)
			{	// and for collapsed graph
				collapfile << *(v->unique_name) << fstr << '\n';
				v->c_vertex_num[0] = cv;
				cv++;
			}
		}
	}
	// now edges, only write if not already written
	for (HGVertex* v : vertices)
	{	for (HGEdge *e : v->incident_s_edges)
		  if (!e->s_written[0])
		  {	e->s_written[0] = 1;
			simplefile << e->vertex1->s_vertex_num[0] << ' ' << e->vertex2->s_vertex_num[0] << ' ';
			e->write_label(simplefile, 0);
			simplefile << '\n';
		  }
		// write edges if vertex is visible...
		if (v->visibility >= 1)
		{	char fstr[57];
			// in traveled graph,
			for (HGEdge *e : v->incident_t_edges)
			  if (!e->t_written[0])
			  {	e->t_written[0] = 1;
				e->traveled_tmg_line(travelfile, fstr, 0, 0, &TravelerList::allusers);
			  }
			if (v->visibility == 2)
			{	// and in collapsed graph
				for (HGEdge *e : v->incident_c_edges)
				  if (!e->c_written[0])
				  {	e->c_written[0] = 1;
					e->collapsed_tmg_line(collapfile, fstr, 0, 0);
				  }
			}
		}
	}
	// traveler names
	for (TravelerList *t : TravelerList::allusers)
		travelfile << t->traveler_name << ' ';
	travelfile << '\n';
	simplefile.close();
	collapfile.close();
	travelfile.close();

	GraphListEntry::entries[0].travelers = 0;
	GraphListEntry::entries[1].travelers = 0;
	GraphListEntry::entries[2].travelers = TravelerList::allusers.size();
}

// write a subset of the data,
// in simple, collapsed and traveled formats,
// restricted by regions in the list if given,
// by systems in the list if given,
// or to within a given area if placeradius is given
void HighwayGraph::write_subgraphs_tmg
(	size_t graphnum, unsigned int threadnum, WaypointQuadtree *qt, ElapsedTime *et, std::mutex *term
)
{	unsigned int cv_count, tv_count;
	#define GRAPH(G) GraphListEntry::entries[graphnum+G]
	std::ofstream simplefile(Args::graphfilepath+'/'+GRAPH(0).filename());
	std::ofstream collapfile(Args::graphfilepath+'/'+GRAPH(1).filename());
	std::ofstream travelfile(Args::graphfilepath+'/'+GRAPH(2).filename());
	std::unordered_set<HGVertex*> mv;
	std::list<HGEdge*> mse, mce, mte;
	std::list<TravelerList*> traveler_lists;
	matching_vertices_and_edges(GRAPH(0), qt, traveler_lists, mv, mse, mce, mte, threadnum, cv_count, tv_count);
	// assign traveler numbers
	unsigned int travnum = 0;
	for (TravelerList *t : traveler_lists)
	{	t->traveler_num[threadnum] = travnum;
		travnum++;
	}
      #ifdef threading_enabled
	term->lock();
	if (GRAPH(0).cat != GraphListEntry::entries[graphnum-1].cat)
		std::cout << '\n' << et->et() << "Writing " << GRAPH(0).category() << " graphs.\n";
      #endif
	std::cout << GRAPH(0).tag()
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
	travelfile << tv_count << ' ' << mte.size() << ' ' << traveler_lists.size() << '\n';

	// write vertices
	unsigned int sv = 0;
	unsigned int cv = 0;
	unsigned int tv = 0;
	for (HGVertex *v : mv)
	{	char fstr[57];
		sprintf(fstr, " %.15g %.15g", v->lat, v->lng);
		// all vertices for simple graph
		simplefile << *(v->unique_name) << fstr << '\n';
		v->s_vertex_num[threadnum] = sv;
		sv++;
		// visible vertices...
		if (v->visibility >= 1)
		{	// for traveled graph,
			travelfile << *(v->unique_name) << fstr << '\n';
			v->t_vertex_num[threadnum] = tv;
			tv++;
			if (v->visibility == 2)
			{	// and for collapsed graph
				collapfile << *(v->unique_name) << fstr << '\n';
				v->c_vertex_num[threadnum] = cv;
				cv++;
			}
		}
	}
	// write edges
	for (HGEdge *e : mse)
	{	simplefile << e->vertex1->s_vertex_num[threadnum] << ' '
			   << e->vertex2->s_vertex_num[threadnum] << ' ';
		e->write_label(simplefile, GRAPH(0).systems);
		simplefile << '\n';
	}
	char fstr[57];
	for (HGEdge *e : mce)
		e->collapsed_tmg_line(collapfile, fstr, threadnum, GRAPH(0).systems);
	for (HGEdge *e : mte)
		e->traveled_tmg_line(travelfile, fstr, threadnum, GRAPH(0).systems, &traveler_lists);
	// traveler names
	for (TravelerList *t : traveler_lists)
		travelfile << t->traveler_name << ' ';
	travelfile << '\n';
	simplefile.close();
	collapfile.close();
	travelfile.close();

	GRAPH(0).vertices = mv.size(); GRAPH(0).edges = mse.size(); GRAPH(0).travelers = 0;
	GRAPH(1).vertices = cv_count;  GRAPH(1).edges = mce.size(); GRAPH(1).travelers = 0;
	GRAPH(2).vertices = tv_count;  GRAPH(2).edges = mte.size(); GRAPH(2).travelers = traveler_lists.size();
	#undef GRAPH
}
