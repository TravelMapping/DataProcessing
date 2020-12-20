class HGVertex
{   /* This class encapsulates information needed for a highway graph
    vertex.
    */
	public:
	double lat, lng;
	const std::string *unique_name;
	char visibility;
	Waypoint *first_waypoint;
	std::unordered_set<Region*> regions;
	std::unordered_set<HighwaySystem*> systems;
	std::list<HGEdge*> incident_s_edges; // simple
	std::list<HGEdge*> incident_c_edges; // collapsed
	std::list<HGEdge*> incident_t_edges; // traveled
	int *s_vertex_num;
	int *c_vertex_num;
	int *t_vertex_num;

	HGVertex(Waypoint *wpt, const std::string *n, unsigned int numthreads)
	{	lat = wpt->lat;
		lng = wpt->lng;
		s_vertex_num = new int[numthreads];
		c_vertex_num = new int[numthreads];
		t_vertex_num = new int[numthreads];
			       // deleted by ~HGVertex, called by HighwayGraph::clear
		unique_name = n;
		visibility = 0;
		    // permitted values:
		    // 0: never visible outside of simple graphs
		    // 1: visible only in traveled graph; hidden in collapsed graph
		    // 2: visible in both traveled & collapsed graphs
		// note: if saving the first waypoint, no longer need
		// lat & lng and can replace with methods
		first_waypoint = wpt;
		if (!wpt->colocated)
		{	if (!wpt->is_hidden) visibility = 2;
			regions.insert(wpt->route->region);
			systems.insert(wpt->route->system);
			wpt->route->region->vertices.insert(this);
			wpt->route->system->vertices.insert(this);
			return;
		}
		for (Waypoint *w : *(wpt->colocated))
		{	// will consider hidden iff all colocated waypoints are hidden
			if (!w->is_hidden) visibility = 2;
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
			DatacheckEntry::add(vis_list.front()->route, vis_list.front()->label, "", "", "VISIBLE_HIDDEN_COLOC",
					     hid_list.front()->route->root+"@"+hid_list.front()->label);
			break;
		  }
	}

	~HGVertex()
	{	//std::cout << "deleting vertex at " << first_waypoint->str() << std::endl;
		while (incident_s_edges.size()) delete incident_s_edges.front();
		while (incident_c_edges.size()) delete incident_c_edges.front();
		delete[] s_vertex_num;
		delete[] c_vertex_num;
		regions.clear();
		systems.clear();
	}
};
