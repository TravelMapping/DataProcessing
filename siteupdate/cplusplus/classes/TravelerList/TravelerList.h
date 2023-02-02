class ConnectedRoute;
class ErrorList;
class HighwaySegment;
class HighwaySystem;
class Region;
class Route;
#include <iostream>
#include <list>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class TravelerList
{   /* This class encapsulates the contents of one .list file
    that represents the travels of one individual user.

    A list file consists of lines of 4 or 6 data fields,
    separated by 1 or more space or tab characters,
    indicating the user has traveled the specified highway
    between the waypoints start_point and end_point.

    Fields starting with a # character are considered comments.
    These and all subsequent fields in a line are ignored,
    and don't count toward the 4 or 6 data fields.

    4 fields describe travel on a chopped route in a single region:
    region route_name start_point end_point

    6 fields describe travel on a connected route in 1 or more regions:
    start_region start_route start_point end_region end_route end_point
    */
	public:
	std::vector<HighwaySegment*> clinched_segments;
	std::string traveler_name;
	std::unordered_map<Region*, double> active_preview_mileage_by_region;				// total mileage per region, active+preview only
	std::unordered_map<Region*, double> active_only_mileage_by_region;				// total mileage per region, active only
	std::unordered_map<HighwaySystem*, std::unordered_map<Region*, double>> system_region_mileages;	// mileage per region per system
	std::unordered_set<Route*> updated_routes;
	std::vector<std::pair<Route*,double>> cr_values;		// for the clinchedRoutes DB table
	std::vector<std::pair<ConnectedRoute*,double>> ccr_values;	// for the clinchedConnectedRoutes DB table
	bool* in_subgraph;
	unsigned int *traveler_num;
	unsigned int active_systems_traveled;
	unsigned int active_systems_clinched;
	unsigned int preview_systems_traveled;
	unsigned int preview_systems_clinched;
	// for locking the allusers list when reading .lists from disk
	// and avoiding data races when creating userlog timestamps
	static std::mutex mtx;
	static std::list<std::string> ids;
	static std::list<std::string>::iterator id_it;
	static std::list<TravelerList*> allusers;
	static std::list<TravelerList*>::iterator tl_it;

	TravelerList(std::string, ErrorList*);
	double active_only_miles();
	double active_preview_miles();
	double system_miles(HighwaySystem *);
	void userlog(const double, const double);
};

bool sort_travelers_by_name(const TravelerList*, const TravelerList*);
