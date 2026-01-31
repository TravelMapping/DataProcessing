#define FMT_HEADER_ONLY
#include "HGEdge.h"
#include "HGVertex.h"
#include "../Args/Args.h"
#include "../HighwaySegment/HighwaySegment.h"
#include "../HighwaySystem/HighwaySystem.h"
#include "../Route/Route.h"
#include "../Waypoint/Waypoint.h"
#include "../../templates/contains.cpp"
#include <fmt/format.h>

HGVertex* HGEdge::v_array;

HGEdge::HGEdge(HighwaySegment *s)
{	// initial construction is based on a HighwaySegment
	vertex1 = s->waypoint1->hashpoint()->vertex;
	vertex2 = s->waypoint2->hashpoint()->vertex;
	format = simple | collapsed | traveled;
	segment_name = s->segment_name();
	vertex1->incident_edges.push_back(this);
	vertex2->incident_edges.push_back(this);
	vertex1->edge_count++;
	vertex2->edge_count++;
	// canonical segment, used to reference region and list of travelers
	// assumption: each edge/segment lives within a unique region
	// and a 'multi-edge' would not be able to span regions as there
	// would be a required visible waypoint at the border
	segment = s;
}

HGEdge::HGEdge(HGVertex *vertex, unsigned char fmt_mask, HGEdge *edge1, HGEdge *edge2)
{	// build by collapsing two existing edges around a common hidden vertex
	c_idx = vertex - v_array;
	format = fmt_mask;
	segment_name = edge1->segment_name;
	/*std::cout << "\nDEBUG: collapsing edges |";
	if (fmt_mask & collapsed) std::cout << 'c';	else std::cout << '-';
	if (fmt_mask & traveled)  std::cout << 't';	else std::cout << '-';
	std::cout << "| along " << segment_name << " at vertex " << *(vertex->unique_name);
	std::cout << "\n       edge1 is " << edge1->str();
	std::cout << "\n       edge2 is " << edge2->str() << std::endl;//*/
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

	// clear format bits of old edges
	edge1->format &= ~fmt_mask;
	edge2->format &= ~fmt_mask;
	// replace edge references at our endpoints with ourself
	if (!edge1->format) edge1->detach();
	if (!edge2->format) edge2->detach();
	vertex1->incident_edges.push_back(this);
	vertex2->incident_edges.push_back(this);
}

void HGEdge::detach()
{	// detach edge from vertices in graph(s) specified in fmt_mask
	auto detach = [&](std::vector<HGEdge*>& v) \
	{ for (std::vector<HGEdge*>::iterator e = v.begin(); e != v.end(); e++) \
	    if (*e == this) \
	    {	v.erase(e); \
		break; \
	    }
	};
	detach(vertex1->incident_edges);
	detach(vertex2->incident_edges);
}

// write line to tmg collapsed edge file
void HGEdge::collapsed_tmg_line(std::ofstream& file, unsigned int threadnum, std::vector<HighwaySystem*> *systems)
{	file << vertex1->c_vertex_num[threadnum] << ' ' << vertex2->c_vertex_num[threadnum] << ' ';
	if (systems)
		segment->write_label(file, systems);
	else	file << segment_name;
	for (HGVertex *intermediate : intermediate_points)
		file << intermediate->coordstr;
	file << '\n';
}

// write line to tmg traveled edge file
void HGEdge::traveled_tmg_line(std::ofstream& file, unsigned int threadnum, std::vector<HighwaySystem*> *systems, bool trav, char* code)
{	file << vertex1->t_vertex_num[threadnum] << ' ' << vertex2->t_vertex_num[threadnum] << ' ';
	if (systems)
		segment->write_label(file, systems);
	else	file << segment_name;
	file << ' ' << (trav ? segment->clinchedby_code(code, threadnum) : "0");
	for (HGVertex *intermediate : intermediate_points)
		file << intermediate->coordstr;
	file << '\n';
}

/* line appropriate for a tmg collapsed edge file, with debug info
std::string HGEdge::debug_tmg_line(std::vector<HighwaySystem*> *systems, unsigned int threadnum)
{	std::string line = std::to_string(vertex1->c_vertex_num[threadnum]) + " [" + *vertex1->unique_name + "] " \
			 + std::to_string(vertex2->c_vertex_num[threadnum]) + " [" + *vertex2->unique_name + "] " + label(systems);
	char fstr[58];
	for (HGVertex *intermediate : intermediate_points)
	{	*fmt::format_to(fstr, "] {:.15} {:.15}", intermediate->lat, intermediate->lng) = 0;
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
	{	*fmt::format_to(fstr, "{:.15} {:.15}", i->lat, i->lng) = 0;
		line += " [" + *i->unique_name + "] " + fstr;
	}
	return line;
}
