#include "HGEdge.h"
#include "HGVertex.h"
#include "HighwayGraph.h"
#include "../Args/Args.h"
#include "../HighwaySegment/HighwaySegment.h"
#include "../HighwaySystem/HighwaySystem.h"
#include "../Route/Route.h"
#include "../Waypoint/Waypoint.h"
#include "../../templates/contains.cpp"

HGEdge::HGEdge(HighwaySegment *s)
{	// initial construction is based on a HighwaySegment
	written = new char[Args::numthreads];
		  // deleted by HGEdge::detach
	// we only need to initialize the first element, for master graphs.
	// the rest will get zeroed out for each subgraph set.
	written[0] = 0;
	vertex1 = s->waypoint1->hashpoint()->vertex;
	vertex2 = s->waypoint2->hashpoint()->vertex;
	format = simple | collapsed | traveled;
	segment_name = s->segment_name();
	vertex1->incident_s_edges.push_back(this);
	vertex2->incident_s_edges.push_back(this);
	vertex1->incident_c_edges.push_back(this);
	vertex2->incident_c_edges.push_back(this);
	vertex1->incident_t_edges.push_back(this);
	vertex2->incident_t_edges.push_back(this);
	// canonical segment, used to reference region and list of travelers
	// assumption: each edge/segment lives within a unique region
	// and a 'multi-edge' would not be able to span regions as there
	// would be a required visible waypoint at the border
	segment = s;
}

HGEdge::HGEdge(HGVertex *vertex, unsigned char fmt_mask)
{	// build by collapsing two existing edges around a common hidden vertex
	written = new char[Args::numthreads];
		  // deleted by HGEdge::detach
	written[0] = 0;
	format = fmt_mask;
	// we know there are exactly 2 incident edges, as we
	// checked for that, and we will replace these two
	// with the single edge we are constructing here...
	HGEdge *edge1 = 0;
	HGEdge *edge2 = 0;
	if (fmt_mask & collapsed)
	{	// ...in the compressed graph, and/or...
		edge1 = vertex->incident_c_edges.front();
		edge2 = vertex->incident_c_edges.back();
	}
	if (fmt_mask & traveled)
	{	// ...in the traveled graph, as appropriate
		edge1 = vertex->incident_t_edges.front();
		edge2 = vertex->incident_t_edges.back();
	}
	// segment names should match as routes should not start or end
	// nor should concurrencies begin or end at a hidden point
	if (edge1->segment_name != edge2->segment_name)
	{	std::cout << "ERROR: segment name mismatch in HGEdge collapse constructor: ";
		std::cout << "edge1 named " << edge1->segment_name << " edge2 named " << edge2->segment_name << '\n' << std::endl;
	}
	segment_name = edge1->segment_name;
	/*std::cout << "\nDEBUG: collapsing edges |";
	if (fmt_mask & collapsed) std::cout << 'c';	else std::cout << '-';
	if (fmt_mask & traveled)  std::cout << 't';	else std::cout << '-';
	std::cout << "| along " << segment_name << " at vertex " << *(vertex->unique_name);
	std::cout << "\n       edge1 is " << edge1->str();
	std::cout << "\n       edge2 is " << edge2->str() << std::endl;//*/
	// segment and route names/systems should also match, but not
	// doing that sanity check here, as the above check should take
	// care of that
	segment = edge1->segment;

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
		intermediate_points.insert(intermediate_points.end(), edge2->intermediate_points.begin(), edge2->intermediate_points.end());
	     }
	else {	//std::cout << "DEBUG: vertex2 getting edge2->vertex1: " << *(edge2->vertex1->unique_name) << " and reversing edge2 intermediates" << std::endl;
		vertex2 = edge2->vertex1;
		intermediate_points.insert(intermediate_points.end(), edge2->intermediate_points.rbegin(), edge2->intermediate_points.rend());
	     }

	//std::cout << "DEBUG: intermediates complete: from " << *(vertex1->unique_name) << " via " << \
			intermediate_point_string() << " to " << *(vertex2->unique_name) << std::endl;
	//std::cout << "DEBUG: new " << str() << std::endl;

	// replace edge references at our endpoints with ourself
	edge1->detach(fmt_mask);
	edge2->detach(fmt_mask);
	if (fmt_mask & collapsed)
	{	vertex1->incident_c_edges.push_back(this);
		vertex2->incident_c_edges.push_back(this);
	}
	if (fmt_mask & traveled)
	{	vertex1->incident_t_edges.push_back(this);
		vertex2->incident_t_edges.push_back(this);
	}
}

void HGEdge::detach() {return detach(format);}
void HGEdge::detach(unsigned char fmt_mask)
{	// detach edge from vertices in graph(s) specified in fmt_mask
	#define DETACH(v) \
	  for (std::list<HGEdge*>::iterator e = v.begin(); e != v.end(); e++) \
	    if (*e == this) \
	    {	v.erase(e); \
		break; \
	    }
	if (fmt_mask & simple)
	{	DETACH(vertex1->incident_s_edges)
		DETACH(vertex2->incident_s_edges)
	}
	if (fmt_mask & collapsed)
	{	DETACH(vertex1->incident_c_edges)
		DETACH(vertex2->incident_c_edges)
	}
	if (fmt_mask & traveled)
	{	DETACH(vertex1->incident_t_edges)
		DETACH(vertex2->incident_t_edges)
	}
	#undef DETACH
	format &= ~fmt_mask;
	if (!format)
	{	delete[] written;
		delete this;
	}
}

// write line to tmg collapsed edge file
void HGEdge::collapsed_tmg_line(std::ofstream& file, char* fstr, unsigned int threadnum, std::list<HighwaySystem*> *systems)
{	file << vertex1->c_vertex_num[threadnum] << ' ' << vertex2->c_vertex_num[threadnum] << ' ';
	segment->write_label(file, systems);
	for (HGVertex *intermediate : intermediate_points)
	{	sprintf(fstr, " %.15g %.15g", intermediate->lat, intermediate->lng);
		file << fstr;
	}
	file << '\n';
}

// write line to tmg traveled edge file
void HGEdge::traveled_tmg_line(std::ofstream& file, char* fstr, unsigned int threadnum, std::list<HighwaySystem*> *systems, bool trav, char* code)
{	file << vertex1->t_vertex_num[threadnum] << ' ' << vertex2->t_vertex_num[threadnum] << ' ';
	segment->write_label(file, systems);
	file << ' ' << (trav ? segment->clinchedby_code(code, threadnum) : "0");
	for (HGVertex *intermediate : intermediate_points)
	{	sprintf(fstr, " %.15g %.15g", intermediate->lat, intermediate->lng);
		file << fstr;
	}
	file << '\n';
}

/* line appropriate for a tmg collapsed edge file, with debug info
std::string HGEdge::debug_tmg_line(std::list<HighwaySystem*> *systems, unsigned int threadnum)
{	std::string line = std::to_string(vertex1->c_vertex_num[threadnum]) + " [" + *vertex1->unique_name + "] " \
			 + std::to_string(vertex2->c_vertex_num[threadnum]) + " [" + *vertex2->unique_name + "] " + label(systems);
	char fstr[58];
	for (HGVertex *intermediate : intermediate_points)
	{	sprintf(fstr, "] %.15g %.15g", intermediate->lat, intermediate->lng);
		line += " [" + *intermediate->unique_name + fstr;
	}
	return line;
}*/

// printable string for this edge
std::string HGEdge::str()
{	std::string str = "HGEdge |";
	if (format & simple)	str += 's';	else str += '-';
	if (format & collapsed)	str += 'c';	else str += '-';
	if (format & traveled)	str += 't';	else str += '-';
	str += "|: " + segment_name
	+ " from " + *vertex1->unique_name
	+  " to "  + *vertex2->unique_name
	+  " via " + std::to_string(intermediate_points.size()) + " points {"
	+ std::to_string((long long unsigned int)this) + '}';
	return str;
}

// return the intermediate points as a string
std::string HGEdge::intermediate_point_string()
{	if (intermediate_points.empty()) return " None";
	std::string line = "";
	char fstr[56];
	for (HGVertex *i : intermediate_points)
	{	sprintf(fstr, "%.15g %.15g", i->lat, i->lng);
		line += " [" + *i->unique_name + "] " + fstr;
	}
	return line;
}
