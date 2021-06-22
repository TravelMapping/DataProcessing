class HGVertex;
class HighwayGraph;
class Route;
#include <deque>
#include <forward_list>
#include <fstream>
#include <list>
#include <string>
#include <unordered_set>
#include <vector>

class Waypoint
{   /* This class encapsulates the information about a single waypoint
    from a .wpt file.

    The line consists of one or more labels, at most one of which can
    be a "regular" label.  Others are "hidden" labels and must begin with
    a '+'.  Then an OSM URL which encodes the latitude and longitude.

    root is the unique identifier for the route in which this waypoint
    is defined
    */

	public:
	Route *route;
	std::list<Waypoint*> *colocated;
	HGVertex *vertex;
	double lat, lng;
	std::string label;
	std::deque<std::string> alt_labels;
	std::vector<Waypoint*> ap_coloc;
	std::forward_list<Waypoint*> near_miss_points;
	unsigned int point_num;
	bool is_hidden;

	Waypoint(char *, Route *);

	std::string str();
	std::string csv_line(unsigned int);
	bool same_coords(Waypoint *);
	bool nearby(Waypoint *, double);
	unsigned int num_colocated();
	double distance_to(Waypoint *);
	double angle(Waypoint *, Waypoint *);
	std::string canonical_waypoint_name(HighwayGraph*);
	std::string simple_waypoint_name();
	bool is_or_colocated_with_active_or_preview();
	std::string root_at_label();
	void nmplogs(std::unordered_set<std::string> &, std::ofstream &, std::list<std::string> &);
	Waypoint* hashpoint();
	bool label_references_route(Route *);

	// Datacheck
	void distance_update(char *, double &, Waypoint *);
	void duplicate_coords(std::unordered_set<Waypoint*> &, char *);
	void label_invalid_char();
	bool label_too_long();
	void out_of_bounds(char *);
	// checks for visible points
	void bus_with_i();
	void interstate_no_hyphen();
	void label_invalid_ends();
	void label_looks_hidden();
	void label_parens();
	void label_selfref(const char *);
	void label_slashes(const char *);
	void lacks_generic();
	void underscore_datachecks(const char *);
	void us_letter();
	void visible_distance(char *, double &, Waypoint *&);
};

bool sort_root_at_label(Waypoint*, Waypoint*);
