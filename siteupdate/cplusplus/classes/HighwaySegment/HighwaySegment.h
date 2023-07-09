class HighwaySystem;
class Route;
class TravelerList;
class Waypoint;
#include "../../templates/TMBitset.cpp"
#include <list>
#include <mutex>

class HighwaySegment
{   /* This class represents one highway segment: the connection between two
    Waypoints connected by one or more routes */

	public:
	Waypoint *waypoint1;
	Waypoint *waypoint2;
	Route *route;
	double length;
	std::list<HighwaySegment*> *concurrent;
	TMBitset<TravelerList*, uint32_t> clinched_by;
	std::mutex clin_mtx;

	HighwaySegment(Waypoint *, Waypoint *, Route *);

	std::string str();
	bool add_clinched_by(size_t);
	std::string csv_line(unsigned int);
	std::string segment_name();
	//std::string concurrent_travelers_sanity_check();
	const char* clinchedby_code(char*, unsigned int);
	bool system_match(std::list<HighwaySystem*>*);
	void write_label(std::ofstream&, std::list<HighwaySystem*> *);
	HighwaySegment* canonical_edge_segment();
};
