class PlaceRadius
{	/* This class encapsulates a place name, file base name, latitude,
	longitude, and radius (in miles) to define the area to which our
	place-based graphs are restricted.
	*/

	public:
	std::string place;
	std::string base;
	double lat;
	double lng;
	unsigned int r;

	PlaceRadius(char *P, char *B, char *Y, char *X, char *R)
	{	place = P;
		base = B;
		lat = strtod(Y, 0);
		lng = strtod(X, 0);
		r = strtoul(R, 0, 10);
	}

	bool contains_vertex(HGVertex *v)
	{	/* return whether v's coordinates are within this area */
		// convert to radians to compute distance
		double rlat1 = lat * (Waypoint::pi/180);
		double rlng1 = lng * (Waypoint::pi/180);
		double rlat2 = v->lat * (Waypoint::pi/180);
		double rlng2 = v->lng * (Waypoint::pi/180);

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

	bool contains_edge(HGEdge *e)
	{	/* return whether both endpoints of edge e are within this area */
		return contains_vertex(e->vertex1) and contains_vertex(e->vertex2);
	}
};
