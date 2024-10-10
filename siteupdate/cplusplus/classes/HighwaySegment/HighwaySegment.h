class HighwaySystem;
class Route;
class TravelerList;
class Waypoint;
#include "../../templates/TMBitset.cpp"
#include <list>
#include <mutex>
#include <vector>

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

	class iterator;

	HighwaySegment(Waypoint*, Route*);
	~HighwaySegment();

	std::string str();
	void add_concurrency(std::ofstream&, Waypoint*);
	//std::string concurrent_travelers_sanity_check();

	// graph generation functions
	std::string segment_name();
	const char* clinchedby_code(char*, unsigned int);
	void write_label(std::ofstream&, std::vector<HighwaySystem*> *);
	HighwaySegment* canonical_edge_segment();
	bool same_ap_routes(HighwaySegment*);
	bool same_vis_routes(HighwaySegment*);
};

class HighwaySegment::iterator
{	HighwaySegment* s;
	decltype(s->concurrent) concurrent;
	decltype(s->concurrent->begin()) i;

	friend bool HighwaySegment::same_ap_routes(HighwaySegment*);

	public:
	iterator(HighwaySegment*);

	HighwaySegment* operator * () const;
	void next_ap();
	void next_vis();
};
