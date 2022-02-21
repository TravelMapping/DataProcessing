class HGVertex;
class HighwayGraph;
class Route;
#include <forward_list>
#include <fstream>
#include <list>
#include <string>
#include <unordered_set>
#include <vector>

class Waypoint
{   /* This class encapsulates the information about a single waypoint
    from a .wpt file.

    The line consists of one or more labels, the first of which is
    a "regular" label; all others are "hidden" labels.
    Then an OSM URL which encodes the latitude and longitude.
    */

	public:
	Route *route;
	std::list<Waypoint*> *colocated;
	HGVertex *vertex;
	double lat, lng;
	std::string label;
	std::vector<std::string> alt_labels;
	std::vector<Waypoint*> ap_coloc;
	std::forward_list<Waypoint*> near_miss_points;
	unsigned int point_num;
	bool is_hidden;

	Waypoint(char *, Route *);

	std::string str();
	std::string csv_line(unsigned int);
	bool same_coords(Waypoint *);
	bool nearby(Waypoint *, double);
	double distance_to(Waypoint *);
	double angle(Waypoint *, Waypoint *);
	std::string canonical_waypoint_name(HighwayGraph*);
	std::string simple_waypoint_name();
	bool is_or_colocated_with_active_or_preview();
	std::string root_at_label();
	void nmplogs(std::unordered_set<std::string> &, std::ofstream &, std::list<std::string> &);
	Waypoint* hashpoint();
	bool label_references_route(Route *);
	Route* coloc_same_number(const char*);
	Route* coloc_same_designation(const std::string&);
	Route* self_intersection();
	bool banner_after_slash(const char*);
	Route* coloc_banner_matches_abbrev();

	// Datacheck
	void distance_update(char *, double &, Waypoint *);
	void label_invalid_char();
	void out_of_bounds(char *);
	// checks for visible points
	void bus_with_i();
	void interstate_no_hyphen();
	void label_invalid_ends();
	void label_looks_hidden();
	void label_parens();
	void label_selfref();
	void label_slashes(const char *);
	void lacks_generic();
	void underscore_datachecks(const char *);
	void us_letter();
	void visible_distance(char *, double &, Waypoint *&);
};

bool sort_root_at_label(Waypoint*, Waypoint*);
