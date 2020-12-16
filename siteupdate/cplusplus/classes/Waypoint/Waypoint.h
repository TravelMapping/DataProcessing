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
	double lat, lng;
	unsigned int point_num;
	std::string label;
	std::deque<std::string> alt_labels;
	std::list<Waypoint*> *colocated;
	std::vector<Waypoint*> ap_coloc;
	std::forward_list<Waypoint*> near_miss_points;
	bool is_hidden;

	Waypoint(char *, Route *);

	std::string str();
	std::string csv_line(unsigned int);
	bool same_coords(Waypoint *);
	bool nearby(Waypoint *, double);
	unsigned int num_colocated();
	double distance_to(Waypoint *);
	double angle(Waypoint *, Waypoint *);
	std::string canonical_waypoint_name(std::list<std::string> &, std::unordered_set<std::string> &);
	std::string simple_waypoint_name();
	bool is_or_colocated_with_active_or_preview();
	std::string root_at_label();
	void nmplogs(std::unordered_set<std::string> &, std::ofstream &, std::list<std::string> &);
	inline Waypoint* hashpoint();
	bool label_references_route(Route *);

	// Datacheck
	inline void distance_update(char *, double &, Waypoint *);
	inline void duplicate_coords(std::unordered_set<Waypoint*> &, char *);
	inline void label_invalid_char();
	inline bool label_too_long();
	inline void out_of_bounds(char *);
	// checks for visible points
	inline void bus_with_i();
	inline void interstate_no_hyphen();
	inline void label_invalid_ends();
	inline void label_looks_hidden();
	inline void label_parens();
	inline void label_selfref(const char *);
	inline void label_slashes(const char *);
	inline void lacks_generic();
	inline void underscore_datachecks(const char *);
	inline void us_letter();
	inline void visible_distance(char *, double &, Waypoint *&);
};
