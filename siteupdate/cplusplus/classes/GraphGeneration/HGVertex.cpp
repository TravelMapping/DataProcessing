#include "HGVertex.h"
#include "../Datacheck/Datacheck.h"
#include "../GraphGeneration/HGEdge.h"
#include "../HighwaySystem/HighwaySystem.h"
#include "../Region/Region.h"
#include "../Route/Route.h"
#include "../Waypoint/Waypoint.h"

HGVertex::HGVertex(Waypoint *wpt, const std::string *n, unsigned int numthreads)
{	lat = wpt->lat;
	lng = wpt->lng;
	wpt->vertex = this;
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
	if (!wpt->colocated)
	{	if (!wpt->is_hidden) visibility = 2;
		wpt->route->region->insert_vertex(this);
		wpt->route->system->insert_vertex(this);
		return;
	}
	for (Waypoint *w : *(wpt->colocated))
	{	// will consider hidden iff all colocated waypoints are hidden
		if (!w->is_hidden) visibility = 2;
		w->route->region->insert_vertex(this);
		w->route->system->insert_vertex(this);
	}
}

HGVertex::~HGVertex()
{	//std::cout << "deleting vertex at " << first_waypoint->str() << std::endl;
	while (incident_s_edges.size()) delete incident_s_edges.front();
	while (incident_c_edges.size()) delete incident_c_edges.front();
	while (incident_t_edges.size()) delete incident_t_edges.front();
	delete[] s_vertex_num;
	delete[] c_vertex_num;
	delete[] t_vertex_num;
}
