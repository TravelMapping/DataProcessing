class HighwayGraphVertexInfo
{   /* This class encapsulates information needed for a highway graph
    vertex.
    */
	public:
	double lat, lng;
	const std::string *unique_name;
	bool is_hidden;
	Waypoint *first_waypoint;
	std::unordered_set<Region*> regions;
	std::unordered_set<HighwaySystem*> systems;
	std::list<HighwayGraphEdgeInfo*> incident_edges;
	std::list<HighwayGraphCollapsedEdgeInfo*> incident_collapsed_edges;
	int *vertex_num;
	int *vis_vertex_num;

	HighwayGraphVertexInfo(Waypoint *wpt, const std::string *n, DatacheckEntryList *datacheckerrors, unsigned int numthreads)
	{	lat = wpt->lat;
		lng = wpt->lng;
		vertex_num = new int[numthreads];
		vis_vertex_num = new int[numthreads];
		unique_name = n;
		// will consider hidden iff all colocated waypoints are hidden
		is_hidden = 1;
		// note: if saving the first waypoint, no longer need
		// lat & lng and can replace with methods
		first_waypoint = wpt;
		if (!wpt->colocated)
		{	if (!wpt->is_hidden) is_hidden = 0;
			regions.insert(wpt->route->region);
			systems.insert(wpt->route->system);
			wpt->route->region->vertices.insert(this);
			wpt->route->system->vertices.insert(this);
			return;
		}
		for (Waypoint *w : *(wpt->colocated))
		{	if (!w->is_hidden) is_hidden = 0;
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

	~HighwayGraphVertexInfo()
	{	//std::cout << "deleting vertex at " << first_waypoint->str() << std::endl;
		while (incident_edges.size()) delete incident_edges.front();
		while (incident_collapsed_edges.size()) delete incident_collapsed_edges.front();
		delete[] vertex_num;
		delete[] vis_vertex_num;
		regions.clear();
		systems.clear();
	}
};
