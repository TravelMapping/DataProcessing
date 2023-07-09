class HGEdge;
class HighwaySystem;
class Region;
class Waypoint;
#include <list>
#include <string>

class HGVertex
{   /* This class encapsulates information needed for a highway graph
    vertex.
    */
	public:
	double lat, lng;
	const std::string *unique_name;
	std::list<HGEdge*> incident_s_edges; // simple
	std::list<HGEdge*> incident_c_edges; // collapsed
	std::list<HGEdge*> incident_t_edges; // traveled
	int *s_vertex_num;
	int *c_vertex_num;
	int *t_vertex_num;
	char visibility;

	void setup(Waypoint*, const std::string*);
	~HGVertex();
};
