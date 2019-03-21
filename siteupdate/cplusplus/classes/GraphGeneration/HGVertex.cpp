class HGVertex
{   /* This class encapsulates information needed for a highway graph
    vertex.
    */
	public:
	double lat, lng;
	const std::string *unique_name;
	char visibility;
	  // 0 = hidden in both collapsed & traveled graphs
	  // 1 = hidden in collapsed graph, visible in traveled graph
	  // 2 = visible in all graphs
	Waypoint *first_waypoint;
	std::unordered_set<Region*> regions;
	std::unordered_set<HighwaySystem*> systems;
	std::list<HGEdge*> incident_s_edges;	// simple
	std::list<HGEdge*> incident_c_edges;	// collapsed
	std::list<HGEdge*> incident_t_edges;	// traveled
	int *s_vertex_num;	// simple
	int *c_vertex_num;	// collapsed
	int *t_vertex_num;	// traveled

	HGVertex(Waypoint *wpt, const std::string *n, DatacheckEntryList *datacheckerrors, unsigned int numthreads)
	{	lat = wpt->lat;
		lng = wpt->lng;
		s_vertex_num = new int[numthreads];
		c_vertex_num = new int[numthreads];
		t_vertex_num = new int[numthreads];
		unique_name = n;
		// will consider hidden iff all colocated waypoints are hidden
		visibility = 0;
		// note: if saving the first waypoint, no longer need
		// lat & lng and can replace with methods
		first_waypoint = wpt;
		if (!wpt->colocated)
		{	if (!wpt->is_hidden) visibility = 2;
			regions.insert(wpt->route->region);	//FIXME review whether still needed
			systems.insert(wpt->route->system);	//FIXME review whether still needed
			wpt->route->region->vertices.insert(this);
			wpt->route->system->vertices.insert(this);
			return;
		}
		for (Waypoint *w : *(wpt->colocated))
		{	if (!w->is_hidden) visibility = 2;
			regions.insert(w->route->region);
			systems.insert(w->route->system);
			w->route->region->vertices.insert(this);
			w->route->system->vertices.insert(this);
		}
		// VISIBLE_HIDDEN_COLOC datacheck
		std::list<Waypoint*>::iterator p = wpt->colocated->begin();
		for (p++; p != wpt->colocated->end(); p++)
		  if ((*p)->is_hidden != wpt->colocated->front()->is_hidden)
		  {	// determine which route, label, and info to use for this entry asciibetically
			std::list<Waypoint*> vis_list;
			std::list<Waypoint*> hid_list;
			for (Waypoint *w : *(wpt->colocated))
			  if (w->is_hidden)
				hid_list.push_back(w);
			  else	vis_list.push_back(w);
			datacheckerrors->add(vis_list.front()->route, vis_list.front()->label, "", "", "VISIBLE_HIDDEN_COLOC",
					     hid_list.front()->route->root+"@"+hid_list.front()->label);
			break;
		  }
	}

	~HGVertex()
	{	//std::cout << "deleting vertex at " << first_waypoint->str() << std::endl;
		while (incident_s_edges.size()) delete incident_s_edges.front();
		while (incident_c_edges.size()) delete incident_c_edges.front();
		while (incident_t_edges.size()) delete incident_t_edges.front();
		delete[] s_vertex_num;
		delete[] c_vertex_num;
		delete[] t_vertex_num;
		regions.clear();
		systems.clear();
	}
};
