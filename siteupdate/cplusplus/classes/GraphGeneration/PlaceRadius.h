class HGEdge;
class HGVertex;
class HighwayGraph;
class WaypointQuadtree;
#include <iostream>
#include <unordered_set>

class PlaceRadius
{	/* This class encapsulates a place name, file base name, latitude,
	longitude, and radius (in miles) to define the area to which our
	place-based graphs are restricted.
	*/

	public:
	std::string descr;	// long description of area, E.G. "New York City"
	std::string title;	// filename title, short name for area, E.G. "nyc"
	double lat, lng;	// center latitude, longitude
	unsigned int r;		// radius in miles

	PlaceRadius(const char *, const char *, double &, double &, int &);

	bool contains_vertex(HGVertex *);
	bool contains_vertex(double, double);
	bool contains_edge(HGEdge *);
	std::unordered_set<HGVertex*> vertices(WaypointQuadtree *, HighwayGraph *);
	std::unordered_set<HGVertex*> v_search(WaypointQuadtree *, HighwayGraph *, double, double);
};
