#include "PlaceRadius.h"
#include "HGEdge.h"
#include "HGVertex.h"
#include "HighwayGraph.h"
#include "../Waypoint/Waypoint.h"
#include "../WaypointQuadtree/WaypointQuadtree.h"
#include <cmath>
#define pi 3.141592653589793238

PlaceRadius::PlaceRadius(const char *D, const char *T, double& Y, double& X, int& R)
{	descr = D;
	title = T;
	lat = Y;
	lng = X;
	r = R;
}

bool PlaceRadius::contains_vertex(HGVertex *v) {return contains_vertex(v->lat, v->lng);}
bool PlaceRadius::contains_vertex(double vlat, double vlng)
{	/* return whether coordinates are within this area */
	// convert to radians to compute distance
	double rlat1 = lat * (pi/180);
	double rlng1 = lng * (pi/180);
	double rlat2 = vlat * (pi/180);
	double rlng2 = vlng * (pi/180);

	/* original formula
	double ans = acos(cos(rlat1)*cos(rlng1)*cos(rlat2)*cos(rlng2) +\
			  cos(rlat1)*sin(rlng1)*cos(rlat2)*sin(rlng2) +\
			  sin(rlat1)*sin(rlat2)) * 3963.1; // EARTH_RADIUS */

	// spherical law of cosines formula (same as orig, with some terms factored out or removed via trig identity)
	double ans = acos(cos(rlat1)*cos(rlat2)*cos(rlng2-rlng1)+sin(rlat1)*sin(rlat2)) * 3963.1; /* EARTH_RADIUS */

	/* Vincenty formula
	double ans = 
	 atan (	sqrt(pow(cos(rlat2)*sin(rlng2-rlng1),2)+pow(cos(rlat1)*sin(rlat2)-sin(rlat1)*cos(rlat2)*cos(rlng2-rlng1),2))
		/
		(sin(rlat1)*sin(rlat2)+cos(rlat1)*cos(rlat2)*cos(rlng2-rlng1))
	      ) * 3963.1; /* EARTH_RADIUS */

	/* haversine formula
	double ans = asin(sqrt(pow(sin((rlat2-rlat1)/2),2) + cos(rlat1) * cos(rlat2) * pow(sin((rlng2-rlng1)/2),2))) * 7926.2; /* EARTH_DIAMETER */

	return ans <= r;
}

bool PlaceRadius::contains_edge(HGEdge *e)
{	/* return whether both endpoints of edge e are within this area */
	return contains_vertex(e->vertex1) and contains_vertex(e->vertex2);
}

std::unordered_set<HGVertex*> PlaceRadius::vertices(WaypointQuadtree *qt, HighwayGraph *g)
{	// Compute and return a set of graph vertices within r miles of (lat, lng).
	// This function handles setup & sanity checks, passing control over
	// to the recursive v_search function to do the actual searching.

	// N/S sanity check: If lat is <= r/2 miles to the N or S pole, lngdelta calculation will fail.
	// In these cases, our place radius will span the entire "width" of the world, from -180 to +180 degrees.
	if (90-fabs(lat)*(pi/180) <= r/7926.2) return v_search(qt, g, -180, +180);

	// width, in degrees longitude, of our bounding box for quadtree search
	double lngdelta = acos((cos(r/3963.1) - pow(sin(lat*(pi/180)),2)) / pow(cos(lat*(pi/180)),2)) / (pi/180);
	double w_bound = lng-lngdelta;
	double e_bound = lng+lngdelta;

	// normal operation; search quadtree within calculated bounds
	std::unordered_set<HGVertex*> vertex_set = v_search(qt, g, w_bound, e_bound);

	// If bounding box spans international date line to west of -180 degrees,
	// search quadtree within the corresponding range of positive longitudes
	if (w_bound <= -180)
	{	while (w_bound <= -180) w_bound += 360;
		for (HGVertex *v : v_search(qt, g, w_bound, 180)) vertex_set.insert(v);
	}

	// If bounding box spans international date line to east of +180 degrees,
	// search quadtree within the corresponding range of negative longitudes
	if (e_bound >= 180)
	{	while (e_bound >= 180) e_bound -= 360;
		for (HGVertex *v : v_search(qt, g, -180, e_bound)) vertex_set.insert(v);
	}

	return vertex_set;
}

std::unordered_set<HGVertex*> PlaceRadius::v_search(WaypointQuadtree *qt, HighwayGraph *g, double w_bound, double e_bound)
{	// recursively search quadtree for waypoints within this PlaceRadius area, and return a set
	// of their corresponding graph vertices to return to the PlaceRadius::vertices function
	std::unordered_set<HGVertex*> vertex_set;

	// first check if this is a terminal quadrant, and if it is,
	// we search for vertices within this quadrant
	if (!qt->refined())
	{	for (Waypoint *p : qt->points)
		  if (	(!p->colocated || p == p->colocated->front())
		  &&	p->is_or_colocated_with_active_or_preview()
		  &&	contains_vertex(p->lat, p->lng)
		     )	vertex_set.insert(p->vertex);
	}
	// if we're not a terminal quadrant, we need to determine which
	// of our child quadrants we need to search and recurse into each
	else {	//printf("DEBUG: recursive case, mid_lat=%.17g mid_lng=%.17g\n", qt->mid_lat, qt->mid_lng); fflush(stdout);
		bool look_n = (lat + r/3963.1/(pi/180)) >= qt->mid_lat;
		bool look_s = (lat - r/3963.1/(pi/180)) <= qt->mid_lat;
		bool look_e = e_bound >= qt->mid_lng;
		bool look_w = w_bound <= qt->mid_lng;
		//std::cout << "DEBUG: recursive case, " << look_n << " " << look_s << " " << look_e << " " << look_w << std::endl;
		// now look in the appropriate child quadrants
		std::unordered_set<HGVertex*> addv;
		if (look_n && look_w)	for (HGVertex *v : v_search(qt->nw_child, g, w_bound, e_bound)) vertex_set.insert(v);
		if (look_n && look_e)	for (HGVertex *v : v_search(qt->ne_child, g, w_bound, e_bound)) vertex_set.insert(v);
		if (look_s && look_w)	for (HGVertex *v : v_search(qt->sw_child, g, w_bound, e_bound)) vertex_set.insert(v);
		if (look_s && look_e)	for (HGVertex *v : v_search(qt->se_child, g, w_bound, e_bound)) vertex_set.insert(v);
	     }
	return vertex_set;
}

#undef pi
