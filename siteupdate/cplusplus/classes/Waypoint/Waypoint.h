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

	Waypoint(char *, Route *, DatacheckEntryList *);

	std::string str();
	std::string csv_line(unsigned int);
	bool same_coords(Waypoint *);
	bool nearby(Waypoint *, double);
	unsigned int num_colocated();
	double distance_to(Waypoint *);
	double angle(Waypoint *, Waypoint *);
	std::string canonical_waypoint_name(std::list<std::string> &, std::unordered_set<std::string> &, DatacheckEntryList *);
	std::string simple_waypoint_name();
	bool is_or_colocated_with_active_or_preview();
	std::string root_at_label();
	void nmplogs(std::unordered_set<std::string> &, std::ofstream &, std::list<std::string> &);
	inline Waypoint* hashpoint();
	bool label_references_route(Route *, DatacheckEntryList *);

	// Datacheck
	inline void distance_update(DatacheckEntryList *, char *, double &, Waypoint *);
	inline void duplicate_coords(DatacheckEntryList *, std::unordered_set<Waypoint*> &, char *);
	inline void label_invalid_char(DatacheckEntryList *);
	inline bool label_too_long(DatacheckEntryList *);
	inline void out_of_bounds(DatacheckEntryList *, char *);
	// checks for visible points
	inline void bus_with_i(DatacheckEntryList *);
	inline void interstate_no_hyphen(DatacheckEntryList *);
	inline void label_invalid_ends(DatacheckEntryList *);
	inline void label_looks_hidden(DatacheckEntryList *);
	inline void label_parens(DatacheckEntryList *);
	inline void label_selfref(DatacheckEntryList *, const char *);
	inline void label_slashes(DatacheckEntryList *, const char *);
	inline void lacks_generic(DatacheckEntryList *);
	inline void underscore_datachecks(DatacheckEntryList *, const char *);
	inline void us_letter(DatacheckEntryList *);
	inline void visible_distance(DatacheckEntryList *, char *, double &, Waypoint *&);
};
