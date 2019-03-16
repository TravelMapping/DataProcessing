HGEdge::HGEdge(HighwaySegment *s, HighwayGraph *graph, bool &duplicate)
{	// temp debug [sic]
	written = 0;
	segment_name = s->segment_name();
	vertex1 = graph->vertices.at(s->waypoint1->hashpoint());
	vertex2 = graph->vertices.at(s->waypoint2->hashpoint());
	// checks for the very unusual cases where an edge ends up
	// in the system as itself and its "reverse"
	for (HGEdge *e : vertex1->incident_edges)
		if (e->vertex1 == vertex2 && e->vertex2 == vertex1)	duplicate = 1;
	for (HGEdge *e : vertex2->incident_edges)
		if (e->vertex1 == vertex2 && e->vertex2 == vertex1)	duplicate = 1;
	if (duplicate)
	{	delete this;
		return;
	}
	vertex1->incident_edges.push_back(this);
	vertex2->incident_edges.push_back(this);
	segment = s;
	s->route->region->edges.insert(this);
	// a list of route name/system pairs
	if (!s->concurrent)
	{	route_names_and_systems.emplace_back(s->route->list_entry_name(), s->route->system);
		s->route->system->edges.insert(this);
	}
	else	for (HighwaySegment *cs : *(s->concurrent))
		{	if (cs->route->system->devel()) continue;
			route_names_and_systems.emplace_back(cs->route->list_entry_name(), cs->route->system);
			cs->route->system->edges.insert(this);
		}
}

HGEdge::~HGEdge()
{	for (	std::list<HGEdge*>::iterator e = vertex1->incident_edges.begin();
		e != vertex1->incident_edges.end();
		e++
	    )	if (*e == this)
		{	vertex1->incident_edges.erase(e);
			break;
		}
	for (	std::list<HGEdge*>::iterator e = vertex2->incident_edges.begin();
		e != vertex2->incident_edges.end();
		e++
	    )	if (*e == this)
		{	vertex2->incident_edges.erase(e);
			break;
		}
	segment_name.clear();
	route_names_and_systems.clear();
}

// compute an edge label, optionally resticted by systems
std::string HGEdge::label(std::list<HighwaySystem*> *systems)
{	std::string the_label;
	for (std::pair<std::string, HighwaySystem*> &ns : route_names_and_systems)
	{	// test whether system in systems
		bool sys_in_sys = 0;
		if (systems) for (HighwaySystem *h : *systems)
		  if (h == ns.second)
		  {	sys_in_sys = 1;
			break;
		  }
		if (!systems || sys_in_sys)
		  if (the_label.empty())
			the_label = ns.first;
		  else	the_label += "," + ns.first;
	}
	return the_label;
}

// printable string for this edge
std::string HGEdge::str()
{	return "HGEdge: " + segment_name + " from " + *vertex1->unique_name + " to " + *vertex2->unique_name;
}
