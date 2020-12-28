class Route;
class TravelerList;
class Waypoint;
#include <list>
#include <mutex>
#include <unordered_set>

class HighwaySegment
{   /* This class represents one highway segment: the connection between two
    Waypoints connected by one or more routes */

	public:
	Waypoint *waypoint1;
	Waypoint *waypoint2;
	Route *route;
	double length;
	std::list<HighwaySegment*> *concurrent;
	std::unordered_set<TravelerList*> clinched_by;
	std::mutex clin_mtx;
	unsigned char system_concurrency_count;
	unsigned char active_only_concurrency_count;
	unsigned char active_preview_concurrency_count;

	HighwaySegment(Waypoint *, Waypoint *, Route *);

	std::string str();
	bool add_clinched_by(TravelerList *);
	std::string csv_line(unsigned int);
	std::string segment_name();
	unsigned int index();
	//std::string concurrent_travelers_sanity_check();
	std::string clinchedby_code(std::list<TravelerList*> *, unsigned int);
	void compute_stats_t(TravelerList*);
};
