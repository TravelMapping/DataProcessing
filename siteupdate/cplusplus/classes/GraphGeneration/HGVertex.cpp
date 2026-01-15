#include "HGVertex.h"
#include "../Args/Args.h"
#include "../Datacheck/Datacheck.h"
#include "../GraphGeneration/HGEdge.h"
#include "../HighwaySystem/HighwaySystem.h"
#include "../Region/Region.h"
#include "../Route/Route.h"
#include "../Waypoint/Waypoint.h"

std::atomic_uint HGVertex::num_hidden(0);
thread_local int* HGVertex::vnums;

void HGVertex::setup(Waypoint *wpt, const std::string *n)
{	lat = wpt->lat;
	lng = wpt->lng;
	wpt->vertex = this;
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
