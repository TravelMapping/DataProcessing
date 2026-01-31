class HGEdge;
class HighwaySystem;
class Region;
class Waypoint;
#include <atomic>
#include <vector>
#include <string>

class HGVertex
{   /* This class encapsulates information needed for a highway graph
    vertex.
    */
	public:
	double lat, lng;
	const std::string *unique_name;
	std::vector<HGEdge*> incident_edges;
	uint16_t edge_count;
	char visibility;
	char coordstr[45]; // only need 43, but alignment requires extra anyway

	static std::atomic_uint num_hidden;
	static thread_local int* vnums;

	void setup(Waypoint*, const std::string*);

	HGEdge* front(unsigned char);
	HGEdge* back (unsigned char);
	void format_coordstr();
};
