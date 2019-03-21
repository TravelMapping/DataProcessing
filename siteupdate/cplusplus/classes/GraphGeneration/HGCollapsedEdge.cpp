HGEdge::HGEdge(HighwaySegment *s, HighwayGraph *graph)
{	// temp debug [sic]
	written = 0;
	segment_name = s->segment_name();
	vertex1 = graph->vertices.at(s->waypoint1->hashpoint());
	vertex2 = graph->vertices.at(s->waypoint2->hashpoint());
	// checks for the very unusual cases where an edge ends up
	// in the system as itself and its "reverse"
	bool duplicate = 0;
	for (HGEdge *e : vertex1->incident_s_edges)
		if (e->vertex1 == vertex2 && e->vertex2 == vertex1)	duplicate = 1;
	for (HGEdge *e : vertex2->incident_s_edges)
		if (e->vertex1 == vertex2 && e->vertex2 == vertex1)	duplicate = 1;
	if (duplicate)
	{	delete this;
		return;
	}
	vertex1->incident_s_edges.push_back(this);
	vertex2->incident_s_edges.push_back(this);
	vertex1->incident_c_edges.push_back(this);
	vertex2->incident_c_edges.push_back(this);
	vertex1->incident_t_edges.push_back(this);
	vertex2->incident_t_edges.push_back(this);
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

HGEdge::HGEdge(HGVertex *vertex)
{	// build by collapsing two existing edges around a common
	// hidden vertex waypoint, whose information is given in
	// vertex
	written = 0;
	// we know there are exactly 2 incident edges, as we
	// checked for that, and we will replace these two
	// with the single edge we are constructing here
	HGEdge *edge1 = vertex->incident_c_edges.front();
	HGEdge *edge2 = vertex->incident_c_edges.back();
	// segment names should match as routes should not start or end
	// nor should concurrencies begin or end at a hidden point
	if (edge1->segment_name != edge2->segment_name)
	{	std::cout << "ERROR: segment name mismatch in HGEdge collapse constructor: ";
		std::cout << "edge1 named " << edge1->segment_name << " edge2 named " << edge2->segment_name << '\n' << std::endl;
	}
	segment_name = edge1->segment_name;
	//std::cout << "\nDEBUG: collapsing edges along " << segment_name << " at vertex " << \
			*(vertex->unique_name) << ", edge1 is " << edge1->str() << " and edge2 is " << edge2->str() << std::endl;
	// segment and route names/systems should also match, but not
	// doing that sanity check here, as the above check should take
	// care of that
	segment = edge1->segment;
	route_names_and_systems = edge1->route_names_and_systems;

	// figure out and remember which endpoints are not the
	// vertex we are collapsing and set them as our new
	// endpoints, and at the same time, build up our list of
	// intermediate vertices
	intermediate_points = edge1->intermediate_points;
	//std::cout << "DEBUG: copied edge1 intermediates" << intermediate_point_string() << std::endl;

	if (edge1->vertex1 == vertex)
	     {	//std::cout << "DEBUG: vertex1 getting edge1->vertex2: " << *(edge1->vertex2->unique_name) << " and reversing edge1 intermediates" << std::endl;
		vertex1 = edge1->vertex2;
		intermediate_points.reverse();
	     }
	else {	//std::cout << "DEBUG: vertex1 getting edge1->vertex1: " << *(edge1->vertex1->unique_name) << std::endl;
		vertex1 = edge1->vertex1;
	     }

	//std::cout << "DEBUG: appending to intermediates: " << *(vertex->unique_name) << std::endl;
	intermediate_points.push_back(vertex);

	if (edge2->vertex1 == vertex)
	     {	//std::cout << "DEBUG: vertex2 getting edge2->vertex2: " << *(edge2->vertex2->unique_name) << std::endl;
		vertex2 = edge2->vertex2;
	     }
	else {	//std:: cout << "DEBUG: vertex2 getting edge2->vertex1: " << *(edge2->vertex1->unique_name) << " and reversing edge2 intermediates" << std::endl;
		vertex2 = edge2->vertex1;
		edge2->intermediate_points.reverse();
	     }
	intermediate_points.splice(intermediate_points.end(), edge2->intermediate_points);

	//std::cout << "DEBUG: intermediates complete: from " << *(vertex1->unique_name) << " via ";
	//std::cout << intermediate_point_string() << " to " << *(vertex2->unique_name) << std::endl;
}

void HGEdge::remove(std::list<HGEdge*> &edge_list)
{	// remove edge from a specified adjacency list
	for (	std::list<HGEdge*>::iterator e = edge_list.begin();
		e != edge_list.end();
		e++
	    )	if (*e == this)
		{	edge_list.erase(e);
			break;
		}
}

HGEdge::~HGEdge()
{	remove(vertex1->incident_s_edges);
	remove(vertex2->incident_s_edges);
	remove(vertex1->incident_c_edges);
	remove(vertex2->incident_c_edges);
	remove(vertex1->incident_t_edges);
	remove(vertex2->incident_t_edges);
	segment_name.clear();
	intermediate_points.clear();
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

// line appropriate for a tmg 1.0 collapsed edge file
std::string HGEdge::collapsed_tmg_line(std::list<HighwaySystem*> *systems, unsigned int threadnum)
{	std::string line = std::to_string(vertex1->c_vertex_num[threadnum]) + " " + std::to_string(vertex2->c_vertex_num[threadnum]) + " " + label(systems);
	char fstr[43];
	for (HGVertex *intermediate : intermediate_points)
	{	sprintf(fstr, " %.15g %.15g", intermediate->lat, intermediate->lng);
		line += fstr;
	}
	return line;
}

// line appropriate for a tmg 2.0 traveled file
std::string HGEdge::traveled_tmg_line(std::list<HighwaySystem*> *systems, unsigned int threadnum, std::list<TravelerList*> *traveler_lists)
{	std::string line = std::to_string(vertex1->t_vertex_num[threadnum]) + " " + std::to_string(vertex2->t_vertex_num[threadnum]) + " " + label(systems);
	line += ' ';
	line += segment->clinchedby_code(traveler_lists);
	char fstr[43];
	for (HGVertex *intermediate : intermediate_points)
	{	sprintf(fstr, " %.15g %.15g", intermediate->lat, intermediate->lng);
		line += fstr;
	}
	return line;
}

// line appropriate for a tmg collapsed edge file, with debug info
std::string HGEdge::debug_tmg_line(std::list<HighwaySystem*> *systems, unsigned int threadnum)
{	std::string line = std::to_string(vertex1->c_vertex_num[threadnum]) + " [" + *vertex1->unique_name + "] " \
			 + std::to_string(vertex2->c_vertex_num[threadnum]) + " [" + *vertex2->unique_name + "] " + label(systems);
	char fstr[44];
	for (HGVertex *intermediate : intermediate_points)
	{	sprintf(fstr, "] %.15g %.15g", intermediate->lat, intermediate->lng);
		line += " [" + *intermediate->unique_name + fstr;
	}
	return line;
}

// printable string for this edge
std::string HGEdge::str()
{	return "HGEdge: " + segment_name
	+ " from " + *vertex1->unique_name
	+  " to "  + *vertex2->unique_name
	+  " via " + std::to_string(intermediate_points.size()) + " points";
}

// return the intermediate points as a string
std::string HGEdge::intermediate_point_string()
{	if (intermediate_points.empty()) return " None";
	std::string line = "";
	char fstr[42];
	for (HGVertex *i : intermediate_points)
	{	sprintf(fstr, "%.15g %.15g", i->lat, i->lng);
		line += " [" + *i->unique_name + "] " + fstr;
	}
	return line;
}

bool HGEdge::s_written()
{	return written & 1;
}

bool HGEdge::c_written()
{	return written & 2;
}

bool HGEdge::t_written()
{	return written & 4;
}
