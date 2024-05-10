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
	int *s_vertex_num;
	int *c_vertex_num;
	int *t_vertex_num;
	uint16_t edge_count;
	char visibility;

	static std::atomic_uint num_hidden;

	void setup(Waypoint*, const std::string*);
	~HGVertex();

	HGEdge* front(unsigned char);
	HGEdge* back (unsigned char);
};
