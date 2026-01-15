class HGVertex;
class HighwaySegment;
class HighwaySystem;
#include <iostream>
#include <list>
#include <vector>

class HGEdge
{   /* This class encapsulates information needed for a highway graph
    edge that can incorporate intermediate points.
    */
	public:
	std::string segment_name;
	HGVertex *vertex1, *vertex2;
	std::list<HGVertex*> intermediate_points; // if more than 1, will go from vertex1 to vertex2
	HighwaySegment *segment;
	uint32_t c_idx; // index of last vertex collapsed, if applicable
			// no "real" use, only for diagnostics & logging
	unsigned char format;

	// constants for more human-readable format masks
	static constexpr unsigned char simple = 1;
	static constexpr unsigned char collapsed = 2;
	static constexpr unsigned char traveled = 4;

	// this avoids adding more arguments to the collapse ctor
	// and adding more ugly code to the collapse routine in the graph ctor
	static HGVertex* v_array;	// for calculating c_idx

	HGEdge(HighwaySegment *);
	HGEdge(HGVertex *, unsigned char, HGEdge*, HGEdge*);

	void detach();
	void collapsed_tmg_line(std::ofstream&, unsigned int, std::vector<HighwaySystem*>*);
	void traveled_tmg_line (std::ofstream&, unsigned int, std::vector<HighwaySystem*>*, bool, char*);
	std::string debug_tmg_line(std::vector<HighwaySystem*> *, unsigned int);
	std::string str();
	std::string intermediate_point_string();
};
