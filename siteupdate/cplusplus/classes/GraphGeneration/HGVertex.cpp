#include "HGVertex.h"
#include "../DatacheckEntry/DatacheckEntry.h"
#include "../GraphGeneration/HGEdge.h"
#include "../HighwaySystem/HighwaySystem.h"
#include "../Region/Region.h"
#include "../Route/Route.h"
#include "../Waypoint/Waypoint.h"

HGVertex::HGVertex(Waypoint *wpt, const std::string *n, unsigned int numthreads)
{	lat = wpt->lat;
	lng = wpt->lng;
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
	// note: if saving the first waypoint, no longer need
	// lat & lng and can replace with methods
	first_waypoint = wpt;
	if (!wpt->colocated)
	{	if (!wpt->is_hidden) visibility = 2;
		wpt->route->region->vertices.insert(this);
		wpt->route->system->vertices.insert(this);
		return;
	}
	for (Waypoint *w : *(wpt->colocated))
	{	// will consider hidden iff all colocated waypoints are hidden
		if (!w->is_hidden) visibility = 2;
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
		DatacheckEntry::add(vis_list.front()->route, vis_list.front()->label, "", "", "VISIBLE_HIDDEN_COLOC",
				     hid_list.front()->route->root+"@"+hid_list.front()->label);
		break;
	  }
}

HGVertex::~HGVertex()
{	//std::cout << "deleting vertex at " << first_waypoint->str() << std::endl;
	while (incident_s_edges.size()) delete incident_s_edges.front();
	while (incident_c_edges.size()) delete incident_c_edges.front();
	delete[] s_vertex_num;
	delete[] c_vertex_num;
}
