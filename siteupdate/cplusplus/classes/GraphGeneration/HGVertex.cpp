#include "HGVertex.h"
#include "../Args/Args.h"
#include "../Datacheck/Datacheck.h"
#include "../GraphGeneration/HGEdge.h"
#include "../HighwaySystem/HighwaySystem.h"
#include "../Region/Region.h"
#include "../Route/Route.h"
#include "../Waypoint/Waypoint.h"

HGVertex::HGVertex(Waypoint *wpt, const std::string *n)
{	lat = wpt->lat;
	lng = wpt->lng;
	wpt->vertex = this;
	s_vertex_num = new int[Args::numthreads];
	c_vertex_num = new int[Args::numthreads];
	t_vertex_num = new int[Args::numthreads];
		       // deleted by ~HGVertex, called by HighwayGraph::clear
	unique_name = n;
	visibility = 0;
	    // permitted values:
	    // 0: never visible outside of simple graphs
	    // 1: visible only in traveled graph; hidden in collapsed graph
	    // 2: visible in both traveled & collapsed graphs
	if (!wpt->colocated)
	{	if (!wpt->is_hidden) visibility = 2;
		wpt->route->region->add_vertex(this);
		wpt->route->system->add_vertex(this);
		return;
	}
	for (Waypoint *w : *(wpt->colocated))
	{	// will consider hidden iff all colocated waypoints are hidden
		if (!w->is_hidden) visibility = 2;
		w->route->region->add_vertex(this);	// Yes, a region/system can get the same vertex multiple times
		w->route->system->add_vertex(this);	// from different routes. But HighwayGraph::matching_vertices_and_edges
	}						// gets rid of any redundancy when making the final set.
}

HGVertex::~HGVertex()
{	//std::cout << "deleting vertex at " << first_waypoint->str() << std::endl;
	while (incident_s_edges.size()) incident_s_edges.front()->detach();
	while (incident_c_edges.size()) incident_c_edges.front()->detach();
	while (incident_t_edges.size()) incident_t_edges.front()->detach();
	delete[] s_vertex_num;
	delete[] c_vertex_num;
	delete[] t_vertex_num;
}
