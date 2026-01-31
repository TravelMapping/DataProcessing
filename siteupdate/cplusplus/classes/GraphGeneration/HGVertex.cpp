#define FMT_HEADER_ONLY
#include "HGVertex.h"
#include "../Args/Args.h"
#include "../Datacheck/Datacheck.h"
#include "../GraphGeneration/HGEdge.h"
#include "../HighwaySystem/HighwaySystem.h"
#include "../Region/Region.h"
#include "../Route/Route.h"
#include "../Waypoint/Waypoint.h"
#include <fmt/format.h>

std::atomic_uint HGVertex::num_hidden(0);

void HGVertex::setup(Waypoint *wpt, const std::string *n)
{	lat = wpt->lat;
	lng = wpt->lng;
	wpt->vertex = this;
	s_vertex_num = new int[Args::numthreads];
	c_vertex_num = new int[Args::numthreads];
	t_vertex_num = new int[Args::numthreads];
		       // deleted by ~HGVertex
	unique_name = n;
	edge_count = 0;
	visibility = 0;
	    // permitted values:
	    // 0: never visible outside of simple graphs
	    // 1: visible only in traveled graph; hidden in collapsed graph
	    // 2: visible in both traveled & collapsed graphs
	if (wpt->colocated)
	{	// will consider hidden iff all colocated waypoints are hidden
		for (Waypoint *w : *(wpt->colocated))
		  if (!w->is_hidden)
		  {	visibility = 2;
			return;
		  }
		num_hidden++;
	}
	else if (!wpt->is_hidden)
		visibility = 2;
	else	num_hidden++;
}

HGVertex::~HGVertex()
{	//std::cout << "deleting vertex at " << first_waypoint->str() << std::endl;
	delete[] s_vertex_num;
	delete[] c_vertex_num;
	delete[] t_vertex_num;
}

HGEdge* HGVertex::front(unsigned char format)
{	for (HGEdge* e : incident_edges)
	  if (e->format & format)
	    return e;
	// Using this as intended with single-format bitmasks,
	// we should never reach this point, because this function
	// will only be called on vertices with edge_count of 2.
	// Nonetheless, let's stop the compiler from complaining.
	throw this;
}

HGEdge* HGVertex::back(unsigned char format)
{	for (auto e = incident_edges.rbegin(); e != incident_edges.rend(); ++e)
	  if ((*e)->format & format)
	    return *e;
	// Using this as intended with single-format bitmasks,
	// we should never reach this point, because this function
	// will only be called on vertices with edge_count of 2.
	// Nonetheless, let's stop the compiler from complaining.
	throw this;
}

void HGVertex::format_coordstr()
{	// sanity checks to avoid buffer overflows via >3 digits to L of decimal point
	while (lat > 90)	lat -= 360;
	while (lat < -90)	lat += 360;
	while (lng >= 540)	lng -= 360;
	while (lng <= -540)	lng += 360;
	*fmt::format_to(coordstr, " {:.15} {:.15}", lat, lng) = 0;
}
