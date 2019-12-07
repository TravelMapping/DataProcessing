class HighwayGraph
{   /* This class implements the capability to create graph
    data structures representing the highway data.

    On construction, build a set of unique vertex names
    and determine edges, at most one per concurrent segment.
    Create three sets of edges:
     - one for the simple graph
     - one for the collapsed graph with hidden waypoints compressed into multi-point edges
     - one for the traveled graph: collapsed edges split at endpoints of users' travels
    */

	public:
	// first, find unique waypoints and create vertex labels
	std::unordered_set<std::string> vertex_names;
	// to track the waypoint name compressions, add log entries
	// to this list
	std::list<std::string> waypoint_naming_log;
	std::unordered_map<Waypoint*, HGVertex*> vertices;

	HighwayGraph
	(	WaypointQuadtree &all_waypoints,
		std::list<HighwaySystem*> &highway_systems,
		DatacheckEntryList *datacheckerrors,
		unsigned int numthreads,
		ElapsedTime &et
	)
	{	// loop for each Waypoint, create a unique name and vertex,
		// unless it's a point not in or colocated with any active
		// or preview system, or is colocated and not at the front
		// of its colocation list
		unsigned int counter = 0;
		std::cout << et.et() + "Creating unique names and vertices" << std::flush;
		for (Waypoint *w : all_waypoints.point_list())
		{	if (counter % 10000 == 0) std::cout << '.' << std::flush;
			counter++;
			// skip if this point is occupied by only waypoints in devel systems
			if (!w->is_or_colocated_with_active_or_preview()) continue;
			// skip if colocated and not at front of list
			if (w->colocated && w != w->colocated->front()) continue;

			// come up with a unique name that brings in its meaning

			// start with the canonical name
			std::string point_name = w->canonical_waypoint_name(waypoint_naming_log);
			bool good_to_go = 1;

			// if that's taken, append the region code
			if (vertex_names.find(point_name) != vertex_names.end())
			{	point_name += "|" + w->route->region->code;
				waypoint_naming_log.push_back("Appended region: " + point_name);
				good_to_go = 0;
			}

			// if that's taken, see if the simple name is available
			if (!good_to_go && vertex_names.find(point_name) != vertex_names.end())
			{	std::string simple_name = w->simple_waypoint_name();
				if (vertex_names.find(simple_name) == vertex_names.end())
				{	waypoint_naming_log.push_back("Revert to simple: " + simple_name + " from (taken) " + point_name);
					point_name = simple_name;
					good_to_go = 1;
				}
				else	good_to_go = 0;
			}

			// if we have not yet succeeded, add !'s until we do
			if (!good_to_go) while (vertex_names.find(point_name) != vertex_names.end())
			{	point_name += "!";
				waypoint_naming_log.push_back("Appended !: " + point_name);
			}

			// we're good; now construct a vertex
			if (!w->colocated)
				vertices[w] = new HGVertex(w, &*(vertex_names.insert(point_name).first), datacheckerrors, numthreads);
			else	vertices[w] = new HGVertex(w->colocated->front(), &*(vertex_names.insert(point_name).first), datacheckerrors, numthreads);
					      // deleted by HighwayGraph::clear
		}
		std::cout << '!' << std::endl;
		//#include "../../debug/unique_names.cpp"

		// create edges
		counter = 0;
		std::cout << et.et() + "Creating edges" << std::flush;
		for (HighwaySystem *h : highway_systems)
		{	if (h->devel()) continue;
			if (counter % 6 == 0) std::cout << '.' << std::flush;
			counter++;
			for (Route &r : h->route_list)
			  for (HighwaySegment *s : r.segment_list)
			    if (!s->concurrent || s == s->concurrent->front())
			      new HGEdge(s, this);
			      // deleted by ~HGVertex, called by HighwayGraph::clear
		}
		std::cout << '!' << std::endl;

		// compress edges adjacent to hidden vertices
		counter = 0;
		std::cout << et.et() + "Compressing collapsed edges" << std::flush;
		for (std::pair<const Waypoint*, HGVertex*> wv : vertices)
		{	if (counter % 10000 == 0) std::cout << '.' << std::flush;
			counter++;
			if (!wv.second->visibility)
			{	// cases with only one edge are flagged as HIDDEN_TERMINUS
				if (wv.second->incident_c_edges.size() < 2)
				{	wv.second->visibility = 2;
					continue;
				}
				// if >2 edges, flag HIDDEN_JUNCTION, mark as visible, and do not compress
				if (wv.second->incident_c_edges.size() > 2)
				{	datacheckerrors->add(wv.first->colocated->front()->route,
							     wv.first->colocated->front()->label,
							     "", "", "HIDDEN_JUNCTION", std::to_string(wv.second->incident_c_edges.size()));
					wv.second->visibility = 2;
					continue;
				}
				// if edge clinched_by sets mismatch, set visibility to 1
				// (visible in traveled graph; hidden in collapsed graph)
				// first, the easy check, for whether set sizes mismatch
				if (wv.second->incident_t_edges.front()->segment->clinched_by.size()
				 != wv.second->incident_t_edges.back()->segment->clinched_by.size())
					wv.second->visibility = 1;
				// next, compare clinched_by sets; look for any element in the 1st not in the 2nd
				else for (TravelerList *t : wv.second->incident_t_edges.front()->segment->clinched_by)
					if (wv.second->incident_t_edges.back()->segment->clinched_by.find(t)
					 == wv.second->incident_t_edges.back()->segment->clinched_by.end())
					{	wv.second->visibility = 1;
						break;
					}
				// construct from vertex this time
				if (wv.second->visibility == 1)
					new HGEdge(wv.second, HGEdge::collapsed);
				else if ((wv.second->incident_c_edges.front() == wv.second->incident_t_edges.front()
				       && wv.second->incident_c_edges.back()  == wv.second->incident_t_edges.back())
				      || (wv.second->incident_c_edges.front() == wv.second->incident_t_edges.back()
				       && wv.second->incident_c_edges.back()  == wv.second->incident_t_edges.front()))
					new HGEdge(wv.second, HGEdge::collapsed | HGEdge::traveled);
				else {	new HGEdge(wv.second, HGEdge::collapsed);
					new HGEdge(wv.second, HGEdge::traveled);
					// Final collapsed edges are deleted by ~HGVertex, called by HighwayGraph::clear.
					// Partially collapsed edges created during the compression process are deleted
					// upon detachment from all graphs.
				     }
			}
		}
		std::cout << '!' << std::endl;
	} // end ctor

	void clear()
	{	for (std::pair<const Waypoint*, HGVertex*> wv : vertices) delete wv.second;
		vertex_names.clear();
		waypoint_naming_log.clear();
		vertices.clear();
	}

	inline void matching_vertices_and_edges
	(	GraphListEntry &g, WaypointQuadtree *qt,
		std::list<TravelerList*> &traveler_lists,
		std::unordered_set<HGVertex*> &mvset,	// final set of vertices matching all criteria
		std::unordered_set<HGEdge*> &mse,	// matching    simple edges
		std::unordered_set<HGEdge*> &mce,	// matching collapsed edges
		std::unordered_set<HGEdge*> &mte,	// matching  traveled edges
		unsigned int &cv_count, unsigned int &tv_count
	)
	{	// Find a set of vertices from the graph, optionally
		// restricted by region or system or placeradius area.
		cv_count = 0;
		tv_count = 0;
		std::unordered_set<HGVertex*> rvset;	// union of all sets in regions
		std::unordered_set<HGVertex*> svset;	// union of all sets in systems
		std::unordered_set<HGVertex*> pvset;	// set of vertices within placeradius
		std::unordered_set<TravelerList*> trav_set;

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
			for (std::pair<const Waypoint*, HGVertex*> wv : vertices)
			  mvset.insert(wv.second);

		// Compute sets of edges for subgraphs, optionally
		// restricted by region or system or placeradius.
		// Keep a count of collapsed & traveled vertices as we go.
		for (HGVertex *v : mvset)
		{	for (HGEdge *e : v->incident_s_edges)
			  if (!g.placeradius || g.placeradius->contains_edge(e))
			    if (!g.regions || list_contains(g.regions, e->segment->route->region))
			    {	bool system_match = !g.systems;
				if (!system_match)
				  for (std::pair<std::string, HighwaySystem*> &rs : e->route_names_and_systems)
				    if (list_contains(g.systems, rs.second))
				    {	system_match = 1;
					break;
				    }
				if (system_match) mse.insert(e);
			    }
			if (v->visibility < 1) continue;
			tv_count++;
			for (HGEdge *e : v->incident_t_edges)
			  if (!g.placeradius || g.placeradius->contains_edge(e))
			    if (!g.regions || list_contains(g.regions, e->segment->route->region))
			    {	bool system_match = !g.systems;
				if (!system_match)
				  for (std::pair<std::string, HighwaySystem*> &rs : e->route_names_and_systems)
				    if (list_contains(g.systems, rs.second))
				    {	system_match = 1;
					break;
				    }
				if (system_match)
				{	mte.insert(e);
					for (TravelerList *t : e->segment->clinched_by) trav_set.insert(t);
				}
			    }
			if (v->visibility < 2) continue;
			cv_count++;
			for (HGEdge *e : v->incident_c_edges)
			  if (!g.placeradius || g.placeradius->contains_edge(e))
			    if (!g.regions || list_contains(g.regions, e->segment->route->region))
			    {	bool system_match = !g.systems;
				if (!system_match)
				  for (std::pair<std::string, HighwaySystem*> &rs : e->route_names_and_systems)
				    if (list_contains(g.systems, rs.second))
				    {	system_match = 1;
					break;
				    }
				if (system_match) mce.insert(e);
			    }
		}
		traveler_lists.assign(trav_set.begin(), trav_set.end());
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
	void write_master_graphs_tmg(std::vector<GraphListEntry> &graph_vector, std::string path, std::list<TravelerList*> &traveler_lists)
	{	std::ofstream simplefile(path+"tm-master-simple.tmg");
		std::ofstream collapfile(path+"tm-master-collapsed.tmg");
		std::ofstream travelfile(path+"tm-master-traveled.tmg");
		simplefile << "TMG 1.0 simple\n";
		collapfile << "TMG 1.0 collapsed\n";
		travelfile << "TMG 2.0 traveled\n";
		simplefile << graph_vector[0].vertices << ' ' << graph_vector[0].edges << '\n';
		collapfile << graph_vector[1].vertices << ' ' << graph_vector[1].edges << '\n';
		travelfile << graph_vector[2].vertices << ' ' << graph_vector[2].edges << ' ' << traveler_lists.size() << '\n';

		// write vertices
		unsigned int sv = 0;
		unsigned int cv = 0;
		unsigned int tv = 0;
		for (std::pair<Waypoint* const, HGVertex*> wv : vertices)
		{	char fstr[57];
			sprintf(fstr, " %.15g %.15g", wv.second->lat, wv.second->lng);
			// all vertices for simple graph
			simplefile << *(wv.second->unique_name) << fstr << '\n';
			wv.second->s_vertex_num[0] = sv;
			sv++;
			// visible vertices...
			if (wv.second->visibility >= 1)
			{	// for traveled graph,
				travelfile << *(wv.second->unique_name) << fstr << '\n';
				wv.second->t_vertex_num[0] = tv;
				tv++;
				if (wv.second->visibility == 2)
				{	// and for collapsed graph
					collapfile << *(wv.second->unique_name) << fstr << '\n';
					wv.second->c_vertex_num[0] = cv;
					cv++;
				}
			}
		}
		// now edges, only write if not already written
		for (std::pair<const Waypoint*, HGVertex*> wv : vertices)
		{	for (HGEdge *e : wv.second->incident_s_edges)
			  if (!e->s_written)
			  {	e->s_written = 1;
				simplefile << e->vertex1->s_vertex_num[0] << ' ' << e->vertex2->s_vertex_num[0] << ' ' << e->label(0) << '\n';
			  }
			// write edges if vertex is visible...
			if (wv.second->visibility >= 1)
			{	// in traveled graph,
				for (HGEdge *e : wv.second->incident_t_edges)
				  if (!e->t_written)
				  {	e->t_written = 1;
					travelfile << e->traveled_tmg_line(0, &traveler_lists, 0) << '\n';
				  }
				if (wv.second->visibility == 2)
				{	// and in collapsed graph
					for (HGEdge *e : wv.second->incident_c_edges)
					  if (!e->c_written)
					  {	e->c_written = 1;
						collapfile << e->collapsed_tmg_line(0, 0) << '\n';
					  }
				}
			}
		}
		// traveler names
		for (TravelerList *t : traveler_lists)
			travelfile << t->traveler_name << ' ';
		travelfile << '\n';
		simplefile.close();
		collapfile.close();
		travelfile.close();

		graph_vector[0].travelers = 0;
		graph_vector[1].travelers = 0;
		graph_vector[2].travelers = traveler_lists.size();
	}

	// write a subset of the data,
	// in simple, collapsed and traveled formats,
	// restricted by regions in the list if given,
	// by systems in the list if given,
	// or to within a given area if placeradius is given
	void write_subgraphs_tmg(	std::vector<GraphListEntry> &graph_vector, std::string path, size_t graphnum,
					unsigned int threadnum, WaypointQuadtree *qt, ElapsedTime *et)
	{	unsigned int cv_count, tv_count;
		std::ofstream simplefile(path+graph_vector[graphnum].filename());
		std::ofstream collapfile(path+graph_vector[graphnum+1].filename());
		std::ofstream travelfile(path+graph_vector[graphnum+2].filename());
		std::unordered_set<HGVertex*> mv;
		std::unordered_set<HGEdge*> mse, mce, mte;
		std::list<TravelerList*> traveler_lists;
		matching_vertices_and_edges(graph_vector[graphnum], qt, traveler_lists, mv, mse, mce, mte, cv_count, tv_count);
		// assign traveler numbers
		unsigned int travnum = 0;
		for (TravelerList *t : traveler_lists)
		{	t->traveler_num[threadnum] = travnum;
			travnum++;
		}
	      #ifdef threading_enabled
		if (graph_vector[graphnum].cat != graph_vector[graphnum-1].cat)
			std::cout << '\n' + et->et() + "Writing " + graph_vector[graphnum].category() + " graphs.\n";
	      #endif
		std::cout << graph_vector[graphnum].tag()
			  << '(' << mv.size() << ',' << mse.size() << ") "
			  << '(' << cv_count << ',' << mce.size() << ") "
			  << '(' << tv_count << ',' << mte.size() << ") " << std::flush;
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
			simplefile << e->vertex1->s_vertex_num[threadnum] << ' '
				   << e->vertex2->s_vertex_num[threadnum] << ' '
				   << e->label(graph_vector[graphnum].systems) << '\n';
		for (HGEdge *e : mce)
			collapfile << e->collapsed_tmg_line(graph_vector[graphnum].systems, threadnum) << '\n';
		for (HGEdge *e : mte)
			travelfile << e->traveled_tmg_line(graph_vector[graphnum].systems, &traveler_lists, threadnum) << '\n';
		// traveler names
		for (TravelerList *t : traveler_lists)
			travelfile << t->traveler_name << ' ';
		travelfile << '\n';
		simplefile.close();
		collapfile.close();
		travelfile.close();

		graph_vector[graphnum].vertices = mv.size();
		graph_vector[graphnum+1].vertices = cv_count;
		graph_vector[graphnum+2].vertices = tv_count;
		graph_vector[graphnum].edges = mse.size();
		graph_vector[graphnum+1].edges = mce.size();
		graph_vector[graphnum+2].edges = mte.size();
		graph_vector[graphnum].travelers = 0;
		graph_vector[graphnum+1].travelers = 0;
		graph_vector[graphnum+2].travelers = traveler_lists.size();
	}
};
