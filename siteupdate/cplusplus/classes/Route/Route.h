class ConnectedRoute;
class ErrorList;
class HighwaySegment;
class HighwaySystem;
class Region;
class TravelerList;
class Waypoint;
class WaypointQuadtree;
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Route
{   /* This class encapsulates the contents of one .csv file line
    that represents a highway within a system and the corresponding
    information from the route's .wpt.

    The format of the .csv file for a highway system is a set of
    semicolon-separated lines, each of which has 8 fields:

    System;Region;Route;Banner;Abbrev;City;Route;AltRouteNames

    The first line names these fields, subsequent lines exist,
    one per highway, with values for each field.

    System: the name of the highway system this route belongs to,
    an instance of HighwaySystem

    Region: the project region or subdivision in which the
    route belongs.

    Route: the route name as would be specified in user lists

    Banner: the (optional) banner on the route such as 'Alt',
    'Bus', or 'Trk'.  Now allowed up to 6 characters

    Abbrev: (optional) for bannered routes or routes in multiple
    sections, the 3-letter abbrevation for the city or other place
    that is used to identify the segment.

    City: (optional) the full name to be displayed for the Abbrev
    above.

    Root: the name of the .wpt file that lists the waypoints of the
    route, without the .wpt extension.

    AltRouteNames: (optional) comma-separated list of former or
    other alternate route names that may appear in user list files.
    */

	public:
	HighwaySystem *system;
	Region *region;		// pointer to a valid Region object
	std::string rg_str;	// region code string, retained for loading files in case no valid object is found
	std::string route;
	std::string banner;
	std::string abbrev;
	std::string city;
	std::string root;
	std::deque<std::string> alt_route_names;
	ConnectedRoute *con_route;

	std::vector<Waypoint*> point_list;
	std::unordered_set<std::string> labels_in_use;
	std::unordered_set<std::string> unused_alt_labels;
	std::unordered_set<std::string> duplicate_labels;
	std::unordered_map<std::string, unsigned int> pri_label_hash, alt_label_hash;
	static std::unordered_map<std::string, Route*> root_hash, pri_list_hash, alt_list_hash;
	static std::mutex awf_mtx;	// for locking the all_wpt_files set when erasing processed WPTs
	std::mutex liu_mtx;	// for locking the labels_in_use set when inserting labels during TravelerList processing
	std::mutex ual_mtx;	// for locking the unused_alt_labels set when removing in-use alt_labels
	std::vector<HighwaySegment*> segment_list;
	std::array<std::string, 5> *last_update;
	double mileage;
	int rootOrder;
	bool is_reversed;

	Route(std::string &, HighwaySystem *, ErrorList &, std::unordered_map<std::string, Region*> &);

	std::string str();
	void read_wpt(WaypointQuadtree *, ErrorList *, std::string, bool, std::unordered_set<std::string> *);
	void print_route();
	HighwaySegment* find_segment_by_waypoints(Waypoint*, Waypoint*);
	std::string chopped_rtes_line();
	std::string csv_line();
	std::string readable_name();
	std::string list_entry_name();
	std::string name_no_abbrev();
	double clinched_by_traveler(TravelerList *);
	//std::string list_line(int, int);
	void write_nmp_merged(std::string);
	inline void store_traveled_segments(TravelerList*, std::ofstream &, unsigned int, unsigned int);
	inline Waypoint* con_beg();
	inline Waypoint* con_end();
};
