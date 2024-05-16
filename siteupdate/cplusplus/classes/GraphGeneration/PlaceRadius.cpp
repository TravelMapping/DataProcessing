#include "PlaceRadius.h"
#include "GraphListEntry.h"
#include "HGEdge.h"
#include "HGVertex.h"
#include "HighwayGraph.h"
#include "../Waypoint/Waypoint.h"
#include "../WaypointQuadtree/WaypointQuadtree.h"
#include <cmath>
#define pi 3.141592653589793238

PlaceRadius::PlaceRadius(const char *D, const char *T, double& Y, double& X, double& R)
{	descr = D;
	title = T;
	lat = Y;
	lng = X;
	r = R;
}

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

void PlaceRadius::matching_ve(TMBitset<HGVertex*,uint64_t>& mv, TMBitset<HGEdge*,uint64_t>& me, WaypointQuadtree *qt)
{	// Compute sets of graph vertices & edges within r miles of (lat, lng).
	// This function handles setup & sanity checks, passing control over
	// to the recursive ve_search function to do the actual searching.

	// N/S sanity check: If lat is <= r/2 miles to the N or S pole, lngdelta calculation will fail.
	// In these cases, our place radius will span the entire "width" of the world, from -180 to +180 degrees.
	if (90-fabs(lat)*(pi/180) <= r/7926.2) return ve_search(mv, me, qt, -180, +180);

	// width, in degrees longitude, of our bounding box for quadtree search
	double lngdelta = acos((cos(r/3963.1) - pow(sin(lat*(pi/180)),2)) / pow(cos(lat*(pi/180)),2)) / (pi/180);
	double w_bound = lng-lngdelta;
	double e_bound = lng+lngdelta;

	// normal operation; search quadtree within calculated bounds
	ve_search(mv, me, qt, w_bound, e_bound);

	// If bounding box spans international date line to west of -180 degrees,
	// search quadtree within the corresponding range of positive longitudes
	if (w_bound <= -180)
	{	do w_bound += 360; while (w_bound <= -180);
		ve_search(mv, me, qt, w_bound, 180);
	}

	// If bounding box spans international date line to east of +180 degrees,
	// search quadtree within the corresponding range of negative longitudes
	if (e_bound >= 180)
	{	do e_bound -= 360; while (e_bound >= 180);
		ve_search(mv, me, qt, -180, e_bound);
	}
}

void PlaceRadius::ve_search(TMBitset<HGVertex*,uint64_t>& mv, TMBitset<HGEdge*,uint64_t>& me,
			    WaypointQuadtree *qt, double w_bound, double e_bound)
{	// recursively search quadtree for waypoints within this PlaceRadius
	// area, and populate a set of their corresponding graph vertices

	// first check if this is a terminal quadrant, and if it is,
	// we search for vertices within this quadrant
	if (!qt->refined())
	{	for (Waypoint *p : qt->points)
		  if (	(!p->colocated || p == p->colocated->front())
		  &&	p->is_or_colocated_with_active_or_preview()
		  &&	contains_vertex(p->lat, p->lng)
		     ){	HGVertex* v = p->vertex;
			mv.add_value(v);
			for (HGEdge* e : v->incident_edges)
			{	HGVertex* v2 = v == e->vertex1 ? e->vertex2 : e->vertex1;
				if (contains_vertex(v2->lat, v2->lng))
				  me.add_value(e);
			}
		      }
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
		if (look_n && look_w)	ve_search(mv, me, qt->nw_child, w_bound, e_bound);
		if (look_n && look_e)	ve_search(mv, me, qt->ne_child, w_bound, e_bound);
		if (look_s && look_w)	ve_search(mv, me, qt->sw_child, w_bound, e_bound);
		if (look_s && look_e)	ve_search(mv, me, qt->se_child, w_bound, e_bound);
	     }
}

#undef pi
