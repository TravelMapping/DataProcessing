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
	std::list<std::string> waypoint_naming_log; //FIXME lose the list; write straight to file
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
		std::cout << et.et() << "Creating unique names and vertices" << std::flush;
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
			// vertices are deleted by HighwayGraph::clear
		}
		std::cout << '!' << std::endl;
		//#include "../../debug/unique_names.cpp"

		// create edges
		counter = 0;
		std::cout << et.et() << "Creating edges" << std::flush;
		for (HighwaySystem *h : highway_systems)
		{	if (h->devel()) continue;
			if (counter % 6 == 0) std::cout << '.' << std::flush;
			counter++;
			for (Route &r : h->route_list)
			  for (HighwaySegment *s : r.segment_list)
			    if (!s->concurrent || s == s->concurrent->front()) new HGEdge(s, this);
		}
		std::cout << '!' << std::endl;

		// compress edges adjacent to hidden vertices
		counter = 0;
		std::cout << et.et() << "Compressing collapsed edges" << std::flush;
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
				{	std::list<Waypoint*>::iterator it = wv.second->first_waypoint->colocated->begin();
					Waypoint *dcw = *it; // "datacheck waypoint"
					for (it++; it != wv.second->first_waypoint->colocated->end(); it++)
					    if (dcw->root_at_label() > (*it)->root_at_label())
						dcw->root_at_label() = (*it)->root_at_label();
					datacheckerrors->add(dcw->route, dcw->label, "", "", "HIDDEN_JUNCTION", std::to_string(wv.second->incident_c_edges.size()));
					wv.second->visibility = 2;
					continue;
				}
				// if edge clinched_by sets mismatch, set visibility to 1
				// (visible in traveled graph; hidden in collapsed graph)
				// first, the easy check, for whether set sizes mismatch
				if (wv.second->incident_c_edges.front()->segment->clinched_by.size()
				 != wv.second->incident_c_edges.back()->segment->clinched_by.size())
				 	wv.second->visibility = 1;
				// next, compare clinched_by sets; look for any element in the 1st not in the 2nd
				else for (TravelerList *t : wv.second->incident_c_edges.front()->segment->clinched_by)
				  if (wv.second->incident_c_edges.back()->segment->clinched_by.find(t)
				   == wv.second->incident_c_edges.back()->segment->clinched_by.end())
				  {	wv.second->visibility = 1;
					break;
				  }
				// construct from vertex this time
				HGEdge *e = new HGEdge(wv.second);
				// collapsed edges are deleted by ~HGVertex, called by HighwayGraph::clear

				// replace edge references at new edge's endpoints with new edge
				// first, edges are always replaced in a collapsed graph
				wv.second->incident_c_edges.front()->remove(e->vertex1->incident_c_edges);
				wv.second->incident_c_edges.front()->remove(e->vertex2->incident_c_edges);
				wv.second->incident_c_edges.back()->remove(e->vertex1->incident_c_edges);
				wv.second->incident_c_edges.back()->remove(e->vertex2->incident_c_edges);
				e->vertex1->incident_c_edges.push_back(e);
				e->vertex2->incident_c_edges.push_back(e);
				// next, collapse the traveler graph where traveler sets match
				if (wv.second->visibility == 1)
				{	wv.second->incident_t_edges.front()->remove(e->vertex1->incident_t_edges);
					wv.second->incident_t_edges.front()->remove(e->vertex2->incident_t_edges);
					wv.second->incident_t_edges.back()->remove(e->vertex1->incident_t_edges);
					wv.second->incident_t_edges.back()->remove(e->vertex2->incident_t_edges);
					e->vertex1->incident_t_edges.push_back(e);
					e->vertex2->incident_t_edges.push_back(e);
				}
			}
		}

		// print summary info
		std::cout << '!' << std::endl;
		std::cout << et.et() << "   Simple graph has " << vertices.size() << " vertices, " << simple_edge_count() << " edges." << std::endl;
		std::cout << et.et() << "Collapsed graph has " << num_collapsed_vertices() << " vertices, " << collapsed_edge_count() << " edges." << std::endl;
		std::cout << et.et() << " Traveled graph has " << num_traveled_vertices() << " vertices, " << traveled_edge_count() << " edges." << std::endl;
	} // end ctor

	unsigned int num_collapsed_vertices()
	{	unsigned int count = 0;
		for (std::pair<const Waypoint*, HGVertex*> wv : vertices)
		  if (wv.second->visibility == 2) count ++;
		return count;
	}

	unsigned int num_traveled_vertices()
	{	unsigned int count = 0;
		for (std::pair<const Waypoint*, HGVertex*> wv : vertices)
		  if (wv.second->visibility) count ++;
		return count;
	}

	unsigned int simple_edge_count()
	{	unsigned int edges = 0;
		for (std::pair<const Waypoint*, HGVertex*> wv : vertices)
		  edges += wv.second->incident_s_edges.size();
		return edges/2;
	}

	unsigned int collapsed_edge_count()
	{	unsigned int edges = 0;
		for (std::pair<const Waypoint*, HGVertex*> wv : vertices)
		  if (wv.second->visibility == 2)
		    edges += wv.second->incident_c_edges.size();
		return edges/2;
	}

	unsigned int traveled_edge_count()
	{	unsigned int edges = 0;
		for (std::pair<const Waypoint*, HGVertex*> wv : vertices)
		  if (wv.second->visibility)
		    edges += wv.second->incident_t_edges.size();
		return edges/2;
	}

	void clear()
	{	for (std::pair<const Waypoint*, HGVertex*> wv : vertices) delete wv.second;
		vertex_names.clear();
		waypoint_naming_log.clear();
		vertices.clear();
	}

	std::unordered_set<HGVertex*> matching_vertices(GraphListEntry &g, unsigned int &cv_count, unsigned int &tv_count)
	{	// return a set of vertices from the graph, optionally
		// restricted by region or system or placeradius area
		cv_count = 0;
		tv_count = 0;
		std::unordered_set<HGVertex*> vertex_set;
		std::unordered_set<HGVertex*> rg_vertex_set;
		std::unordered_set<HGVertex*> sys_vertex_set;
		// rg_vertex_set is the union of all sets in regions
		if (g.regions) for (Region *r : *g.regions)
			rg_vertex_set.insert(r->vertices.begin(), r->vertices.end());
		// sys_vertex_set is the union of all sets in systems
		if (g.systems) for (HighwaySystem *h : *g.systems)
			sys_vertex_set.insert(h->vertices.begin(), h->vertices.end());

		// determine which vertices are within our region(s) and/or system(s)
		if (g.regions)
		     {	vertex_set = rg_vertex_set;
			if (g.systems)
				// both regions & systems populated: vertex_set is
				// intersection of rg_vertex_set & sys_vertex_set
				for (HGVertex *v : sys_vertex_set)
				  vertex_set.erase(v);
		     }
		else	if (g.systems)
				// only systems populated
				vertex_set = sys_vertex_set;
			else	// neither are populated; include all vertices...
			  for (std::pair<const Waypoint*, HGVertex*> wv : vertices)
				// ...unless a PlaceRadius is specified
				if (!g.placeradius || g.placeradius->contains_vertex_info(wv.second))
				  vertex_set.insert(wv.second);

		// if placeradius is provided along with region or
		// system parameters, erase vertices outside placeradius
		if (g.placeradius && (g.systems || g.regions))
		{	std::unordered_set<HGVertex*>::iterator v = vertex_set.begin();
			while(v != vertex_set.end())
			  if (!g.placeradius->contains_vertex_info(*v))
				v = vertex_set.erase(v);
			  else	v++;
		}
		// find number of visible vertices
		for (HGVertex *v : vertex_set)
		{	if (v->visibility >= 1) tv_count++;
			if (v->visibility == 2) cv_count++;
		}
		return vertex_set;
	}//*/

	std::unordered_set<HGEdge*> matching_simple_edges(std::unordered_set<HGVertex*> &mv, GraphListEntry &g)
	{	// return a set of edges from the graph, optionally
		// restricted by region or system or placeradius area
		std::unordered_set<HGEdge*> edge_set;
		std::unordered_set<HGEdge*> rg_edge_set;
		std::unordered_set<HGEdge*> sys_edge_set;
		// rg_edge_set is the union of all sets in regions
		if (g.regions) for (Region *r : *g.regions)
			rg_edge_set.insert(r->edges.begin(), r->edges.end());
		// sys_edge_set is the union of all sets in systems
		if (g.systems) for (HighwaySystem *h : *g.systems)
			sys_edge_set.insert(h->edges.begin(), h->edges.end());

		// determine which edges are within our region(s) and/or system(s)
		if (g.regions)
		     {	edge_set = rg_edge_set;
			if (g.systems)
				// both regions & systems populated: edge_set is
				// intersection of rg_edge_set & sys_edge_set
				for (HGEdge *e : sys_edge_set)
				  edge_set.erase(e);
		     }
		else	if (g.systems)
				// only systems populated
				edge_set = sys_edge_set;
			else	// neither are populated; include all edges...
			  for (HGVertex *v : mv)
			    for (HGEdge *e : v->incident_s_edges)
				// ...unless a PlaceRadius is specified
				if (!g.placeradius || g.placeradius->contains_edge(e))
				  edge_set.insert(e);

		// if placeradius is provided along with non-empty region
		// or system parameters, erase edges outside placeradius
		if (g.placeradius && (g.systems || g.regions))
		{	std::unordered_set<HGEdge*>::iterator e = edge_set.begin();
			while(e != edge_set.end())
			  if (!g.placeradius->contains_edge(*e))
				e = edge_set.erase(e);
			  else	e++;
		}
		return edge_set;
	}

	std::unordered_set<HGEdge*> matching_collapsed_edges(std::unordered_set<HGVertex*> &mv, GraphListEntry &g)
	{	// return a set of edges from the graph edges for the collapsed
		// edge format, optionally restricted by region or system or
		// placeradius area
		std::unordered_set<HGEdge*> edge_set;
		for (HGVertex *v : mv)
		{	if (v->visibility < 2) continue;
			for (HGEdge *e : v->incident_c_edges)
			  if (!g.placeradius || g.placeradius->contains_edge(e))
			  {	bool rg_in_rg = 0;
				if (g.regions) for (Region *r : *g.regions)
				  if (r == e->segment->route->region)
				  {	rg_in_rg = 1;
					break;
				  }
				if (!g.regions || rg_in_rg)
				{	bool system_match = !g.systems;
					if (!system_match)
					  for (std::pair<std::string, HighwaySystem*> &rs : e->route_names_and_systems)
					  {	bool sys_in_sys = 0;
						if (g.systems) for (HighwaySystem *s : *g.systems)
						  if (s == rs.second)
						  {	sys_in_sys = 1;
							break;
						  }
						if (sys_in_sys) system_match = 1;
					  }
					if (system_match) edge_set.insert(e);
				}
			  }
		}
		return edge_set;
	}

	std::unordered_set<HGEdge*> matching_traveled_edges(std::unordered_set<HGVertex*> &mv, GraphListEntry &g)
	{	// return a set of edges from the graph edges for the traveled
		// graph format, optionally restricted by region or system or
		// placeradius area
		std::unordered_set<HGEdge*> edge_set;
		for (HGVertex *v : mv)
		{	if (!v->visibility) continue;
			for (HGEdge *e : v->incident_t_edges)
			  if (!g.placeradius || g.placeradius->contains_edge(e))
			  {	bool rg_in_rg = 0;
				if (g.regions) for (Region *r : *g.regions)
				  if (r == e->segment->route->region)
				  {	rg_in_rg = 1;
					break;
				  }
				if (!g.regions || rg_in_rg)
				{	bool system_match = !g.systems;
					if (!system_match)
					  for (std::pair<std::string, HighwaySystem*> &rs : e->route_names_and_systems)
					  {	bool sys_in_sys = 0;
						if (g.systems) for (HighwaySystem *s : *g.systems)
						  if (s == rs.second)
						  {	sys_in_sys = 1;
							break;
						  }
						if (sys_in_sys) system_match = 1;
					  }
					if (system_match) edge_set.insert(e);
				}
			  }
		}
		return edge_set;
	}

	// write the entire set of highway data in .tmg format.
	// The first line is a header specifying
	// the format and version number, the second line specifying the
	// number of waypoints, w, and the number of connections, c, then w
	// lines describing waypoints (label, latitude, longitude), then c
	// lines describing connections (endpoint 1 number, endpoint 2
	// number, route label)
	//
	// passes number of vertices and number of edges written by reference
	//
	void write_master_tmg_simple(GraphListEntry *msptr, std::string filename)
	{	std::ofstream tmgfile(filename.data());
		tmgfile << "TMG 1.0 simple\n";
		tmgfile << vertices.size() << ' ' << simple_edge_count() << '\n';
		// number waypoint entries as we go to support original .gra
		// format output
		int vertex_num = 0;
		for (std::pair<const Waypoint*, HGVertex*> wv : vertices)
		{	char fstr[42];
			sprintf(fstr, "%.15g %.15g", wv.second->lat, wv.second->lng);
			tmgfile << *(wv.second->unique_name) << ' ' << fstr << '\n';
			wv.second->s_vertex_num[0] = vertex_num;
			vertex_num++;
		}
		// sanity check
		if (vertices.size() != vertex_num)
			std::cout << "ERROR: computed " << vertices.size() << " waypoints but wrote " << vertex_num << std::endl;

		// now edges, only print if not already printed
		int edge = 0;
		for (std::pair<const Waypoint*, HGVertex*> wv : vertices)
		  for (HGEdge *e : wv.second->incident_s_edges)
		    if (!e->s_written())
		    {	e->written |= 1;
			tmgfile << e->vertex1->s_vertex_num[0] << ' ' << e->vertex2->s_vertex_num[0] << ' ' << e->label(0) << '\n';
			edge++;
		    }
		// sanity checks
		for (std::pair<const Waypoint*, HGVertex*> wv : vertices)
		  for (HGEdge *e : wv.second->incident_s_edges)
		    if (!e->s_written())
		      std::cout << "ERROR: never wrote edge " << e->vertex1->s_vertex_num << ' ' << e->vertex2->s_vertex_num[0] << ' ' << e->label(0) << std::endl;
		//if (simple_edge_count() != edge)
		  std::cout << "ERROR: computed " << simple_edge_count() << " edges but wrote " << edge << std::endl;

		tmgfile.close();
		msptr->vertices = vertices.size();
		msptr->edges = simple_edge_count();
	}

	// write the entire set of data in the tmg collapsed edge format
	void write_master_tmg_collapsed(GraphListEntry *mcptr, std::string filename, unsigned int threadnum, std::list<TravelerList*> *traveler_lists)
	{	std::ofstream tmgfile(filename.data());
		unsigned int num_collapsed_edges = collapsed_edge_count();
		tmgfile << "TMG 1.0 collapsed\n";
		tmgfile << num_collapsed_vertices() << " " << num_collapsed_edges << '\n';

		// write visible vertices
		int c_vertex_num = 0;
		for (std::pair<const Waypoint*, HGVertex*> wv : vertices)
		  if (wv.second->visibility == 2)
		  {	char fstr[42];
			sprintf(fstr, "%.15g %.15g", wv.second->lat, wv.second->lng);
			tmgfile << *(wv.second->unique_name) << ' ' << fstr << '\n';
			wv.second->c_vertex_num[threadnum] = c_vertex_num;
			c_vertex_num++;
		  }
		// write collapsed edges
		int edge = 0;
		for (std::pair<const Waypoint*, HGVertex*> wv : vertices)
		  if (wv.second->visibility == 2)
		    for (HGEdge *e : wv.second->incident_c_edges)
		      if (!e->c_written())
		      {	e->written |= 2;
			tmgfile << e->collapsed_tmg_line(0, threadnum) << '\n';
			edge++;
		      }
		// sanity check on edges written
		//if (num_collapsed_edges != edge)
			std::cout << "ERROR: computed " << num_collapsed_edges << " collapsed edges, but wrote " << edge << '\n';

		tmgfile.close();
		mcptr->vertices = num_collapsed_vertices();
		mcptr->edges = num_collapsed_edges;
	}

	// write the entire set of data in the tmg traveled format
	void write_master_tmg_traveled(GraphListEntry *mtptr, std::string filename, unsigned int threadnum, std::list<TravelerList*> *traveler_lists)
	{	std::ofstream tmgfile(filename.data());
		unsigned int num_traveled_edges = traveled_edge_count();
		tmgfile << "TMG 2.0 traveled\n";
		tmgfile << num_traveled_vertices() << " " << num_traveled_edges << '\n';

		// write visible vertices
		int t_vertex_num = 0;
		for (std::pair<const Waypoint*, HGVertex*> wv : vertices)
		  if (wv.second->visibility)
		  {	char fstr[42];
			sprintf(fstr, "%.15g %.15g", wv.second->lat, wv.second->lng);
			tmgfile << *(wv.second->unique_name) << ' ' << fstr << '\n';
			wv.second->t_vertex_num[threadnum] = t_vertex_num;
			t_vertex_num++;
		  }
		// write traveled edges
		int edge = 0;
		for (std::pair<const Waypoint*, HGVertex*> wv : vertices)
		  if (wv.second->visibility)
		    for (HGEdge *e : wv.second->incident_t_edges)
		      if (!e->t_written())
		      {	e->written |= 4;
			tmgfile << e->traveled_tmg_line(0, threadnum, traveler_lists) << '\n';
			edge++;
		      }
		// sanity check on edges written
		//if (num_traveled_edges != edge)
			std::cout << "ERROR: computed " << num_traveled_edges << " traveled edges, but wrote " << edge << '\n';//*/

		tmgfile.close();
		mtptr->vertices = num_traveled_vertices();
		mtptr->edges = num_traveled_edges;
	}

	// write a subset of the data,
	// in both simple and collapsed formats,
	// restricted by regions in the list if given,
	// by system in the list if given,
	// or to within a given area if placeradius is given
	void write_subgraphs_tmg(std::vector<GraphListEntry> &graph_vector, std::string path, size_t graphnum, unsigned int threadnum,
				 std::list<TravelerList*> *traveler_lists)
	{	unsigned int cv_count, tv_count;
		std::string simplefilename = path+graph_vector[graphnum].filename();
		std::string collapfilename = path+graph_vector[graphnum+1].filename();
		std::string travelfilename = path+graph_vector[graphnum+2].filename();
		std::ofstream simplefile(simplefilename.data());
		std::ofstream collapfile(collapfilename.data());
		std::ofstream travelfile(collapfilename.data());
		std::unordered_set<HGVertex*> mv = matching_vertices(graph_vector[graphnum], cv_count, tv_count);
		std::unordered_set<HGEdge*> mse = matching_simple_edges(mv, graph_vector[graphnum]);
		std::unordered_set<HGEdge*> mce = matching_collapsed_edges(mv, graph_vector[graphnum]);
		std::unordered_set<HGEdge*> mte = matching_traveled_edges(mv, graph_vector[graphnum]);
		std::cout << graph_vector[graphnum].tag()
			  << '(' << mv.size() << ',' << mse.size() << ") "
			  << '(' << cv_count << ',' << mce.size() << ") "
			  << '(' << tv_count << ',' << mte.size() << ") " << std::flush;
		simplefile << "TMG 1.0 simple\n";
		collapfile << "TMG 1.0 collapsed\n";
		travelfile << "TMG 2.0 traveled\n";
		simplefile << mv.size() << ' ' << mse.size() << '\n';
		collapfile << cv_count << ' ' << mce.size() << '\n';
		travelfile << tv_count << ' ' << mte.size() << '\n';

		// write vertices
		unsigned int sv = 0;
		unsigned int cv = 0;
		unsigned int tv = 0;
		for (HGVertex *v : mv)
		{	char fstr[43];
			sprintf(fstr, " %.15g %.15g", v->lat, v->lng);
			// all vertices, for simple graph
			simplefile << *(v->unique_name) << fstr << '\n';
			v->s_vertex_num[threadnum] = sv;
			sv++;
			// visible vertices...
			if (v->visibility)
			{	// ...for traveled graph,
				travelfile << *(v->unique_name) << fstr << '\n';
				v->t_vertex_num[threadnum] = tv;
				tv++;
				// ...and collapsed graph
				if (v->visibility == 2)
				{	collapfile << *(v->unique_name) << fstr << '\n';
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
			travelfile << e->traveled_tmg_line(graph_vector[graphnum].systems, threadnum, traveler_lists) << '\n';
		for (TravelerList *t : *traveler_lists)
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
	}
};
