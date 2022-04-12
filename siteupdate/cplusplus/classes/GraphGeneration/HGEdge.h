class HGVertex;
class HighwaySegment;
class HighwaySystem;
class TravelerList;
#include <iostream>
#include <list>

class HGEdge
{   /* This class encapsulates information needed for a highway graph
    edge that can incorporate intermediate points.
    */
	public:
	char *written;
	std::string segment_name;
	HGVertex *vertex1, *vertex2;
	std::list<HGVertex*> intermediate_points; // if more than 1, will go from vertex1 to vertex2
	HighwaySegment *segment;
	unsigned char format;

	// constants for more human-readable format masks
	static const unsigned char simple = 1;
	static const unsigned char collapsed = 2;
	static const unsigned char traveled = 4;

	HGEdge(HighwaySegment *);
	HGEdge(HGVertex *, unsigned char);

	void detach();
	void detach(unsigned char);
	void collapsed_tmg_line(std::ofstream&, char*, unsigned int, std::list<HighwaySystem*>*);
	void traveled_tmg_line (std::ofstream&, char*, unsigned int, std::list<HighwaySystem*>*, std::list<TravelerList*>*, char*);
	std::string debug_tmg_line(std::list<HighwaySystem*> *, unsigned int);
	std::string str();
	std::string intermediate_point_string();
};
