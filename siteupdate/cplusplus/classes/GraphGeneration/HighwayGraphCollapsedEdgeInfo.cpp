HighwayGraphCollapsedEdgeInfo::HighwayGraphCollapsedEdgeInfo(HighwayGraphEdgeInfo *e)
{	// initial construction is based on a HighwayGraphEdgeInfo
	written = 0;
	segment_name = e->segment_name;
	vertex1 = e->vertex1;
	vertex2 = e->vertex2;
	// assumption: each edge/segment lives within a unique region
	// and a 'multi-edge' would not be able to span regions as there
	// would be a required visible waypoint at the border
	region = e->region;
	// a list of route name/system pairs
	route_names_and_systems = e->route_names_and_systems;
	vertex1->incident_collapsed_edges.push_back(this);
	vertex2->incident_collapsed_edges.push_back(this);
}

HighwayGraphCollapsedEdgeInfo::HighwayGraphCollapsedEdgeInfo(HighwayGraphVertexInfo *vertex_info)
{	// build by collapsing two existing edges around a common
	// hidden vertex waypoint, whose information is given in
	// vertex_info
	written = 0;
	// we know there are exactly 2 incident edges, as we
	// checked for that, and we will replace these two
	// with the single edge we are constructing here
	HighwayGraphCollapsedEdgeInfo *edge1 = vertex_info->incident_collapsed_edges.front();
	HighwayGraphCollapsedEdgeInfo *edge2 = vertex_info->incident_collapsed_edges.back();
	// segment names should match as routes should not start or end
	// nor should concurrencies begin or end at a hidden point
	if (edge1->segment_name != edge2->segment_name)
	{	std::cout << "ERROR: segment name mismatch in HighwayGraphCollapsedEdgeInfo: ";
		std::cout << "edge1 named " << edge1->segment_name << " edge2 named " << edge2->segment_name << '\n' << std::endl;
	}
	segment_name = edge1->segment_name;
	//std::cout << "\nDEBUG: collapsing edges along " << segment_name << " at vertex " << \
			*(vertex_info->unique_name) << ", edge1 is " << edge1->str() << " and edge2 is " << edge2->str() << std::endl;
	// region and route names/systems should also match, but not
	// doing that sanity check here, as the above check should take
	// care of that
	region = edge1->region;
	route_names_and_systems = edge1->route_names_and_systems;

	// figure out and remember which endpoints are not the
	// vertex we are collapsing and set them as our new
	// endpoints, and at the same time, build up our list of
	// intermediate vertices
	intermediate_points = edge1->intermediate_points;
	//std::cout << "DEBUG: copied edge1 intermediates" << intermediate_point_string() << std::endl;

	if (edge1->vertex1 == vertex_info)
	     {	//std::cout << "DEBUG: vertex1 getting edge1->vertex2: " << *(edge1->vertex2->unique_name) << " and reversing edge1 intermediates" << std::endl;
		vertex1 = edge1->vertex2;
		intermediate_points.reverse();
	     }
	else {	//std::cout << "DEBUG: vertex1 getting edge1->vertex1: " << *(edge1->vertex1->unique_name) << std::endl;
		vertex1 = edge1->vertex1;
	     }

	//std::cout << "DEBUG: appending to intermediates: " << *(vertex_info->unique_name) << std::endl;
	intermediate_points.push_back(vertex_info);

	if (edge2->vertex1 == vertex_info)
	     {	//std::cout << "DEBUG: vertex2 getting edge2->vertex2: " << *(edge2->vertex2->unique_name) << std::endl;
		vertex2 = edge2->vertex2;
	     }
	else {	//std:: cout << "DEBUG: vertex2 getting edge2->vertex1: " << *(edge2->vertex1->unique_name) << " and reversing edge2 intermediates" << std::endl;
		vertex2 = edge2->vertex1;
		edge2->intermediate_points.reverse();
	     }
	intermediate_points.splice(intermediate_points.end(), edge2->intermediate_points);

	//std::cout << "DEBUG: intermediates complete: from " << *(vertex1->unique_name) << " via " \
		  << intermediate_point_string() << " to " << *(vertex2->unique_name) << std::endl;

	// replace edge references at our endpoints with ourself
	delete edge1; // destructor removes edge from adjacency lists
	delete edge2; // destructor removes edge from adjacency lists
	vertex1->incident_collapsed_edges.push_back(this);
	vertex2->incident_collapsed_edges.push_back(this);
}

HighwayGraphCollapsedEdgeInfo::~HighwayGraphCollapsedEdgeInfo()
{	for (	std::list<HighwayGraphCollapsedEdgeInfo*>::iterator e = vertex1->incident_collapsed_edges.begin();
		e != vertex1->incident_collapsed_edges.end();
		e++
	    )	if (*e == this)
		{	vertex1->incident_collapsed_edges.erase(e);
			break;
		}
	for (	std::list<HighwayGraphCollapsedEdgeInfo*>::iterator e = vertex2->incident_collapsed_edges.begin();
		e != vertex2->incident_collapsed_edges.end();
		e++
	    )	if (*e == this)
		{	vertex2->incident_collapsed_edges.erase(e);
			break;
		}
	segment_name.clear();
	intermediate_points.clear();
	route_names_and_systems.clear();
}

// compute an edge label, optionally resticted by systems
std::string HighwayGraphCollapsedEdgeInfo::label(std::list<HighwaySystem*> *systems)
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

// line appropriate for a tmg collapsed edge file
std::string HighwayGraphCollapsedEdgeInfo::collapsed_tmg_line(std::list<HighwaySystem*> *systems, unsigned int threadnum)
{	std::string line = std::to_string(vertex1->vis_vertex_num[threadnum]) + " " + std::to_string(vertex2->vis_vertex_num[threadnum]) + " " + label(systems);
	char fstr[43];
	for (HighwayGraphVertexInfo *intermediate : intermediate_points)
	{	sprintf(fstr, " %.15g %.15g", intermediate->lat, intermediate->lng);
		line += fstr;
	}
	return line;
}

// line appropriate for a tmg collapsed edge file, with debug info
std::string HighwayGraphCollapsedEdgeInfo::debug_tmg_line(std::list<HighwaySystem*> *systems, unsigned int threadnum)
{	std::string line = std::to_string(vertex1->vis_vertex_num[threadnum]) + " [" + *vertex1->unique_name + "] " \
			 + std::to_string(vertex2->vis_vertex_num[threadnum]) + " [" + *vertex2->unique_name + "] " + label(systems);
	char fstr[44];
	for (HighwayGraphVertexInfo *intermediate : intermediate_points)
	{	sprintf(fstr, "] %.15g %.15g", intermediate->lat, intermediate->lng);
		line += " [" + *intermediate->unique_name + fstr;
	}
	return line;
}

// printable string for this edge
std::string HighwayGraphCollapsedEdgeInfo::str()
{	return "HighwayGraphCollapsedEdgeInfo: " + segment_name
	+ " from " + *vertex1->unique_name
	+  " to "  + *vertex2->unique_name
	+  " via " + std::to_string(intermediate_points.size()) + " points";
}

// return the intermediate points as a string
std::string HighwayGraphCollapsedEdgeInfo::intermediate_point_string()
{	if (intermediate_points.empty()) return " None";
	std::string line = "";
	char fstr[42];
	for (HighwayGraphVertexInfo *i : intermediate_points)
	{	sprintf(fstr, "%.15g %.15g", i->lat, i->lng);
		line += " [" + *i->unique_name + "] " + fstr;
	}
	return line;
}
