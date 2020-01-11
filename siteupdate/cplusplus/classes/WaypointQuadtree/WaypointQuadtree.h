class WaypointQuadtree
{	// This class defines a recursive quadtree structure to store
	// Waypoint objects for efficient geometric searching.

	public:
	double min_lat, min_lng, max_lat, max_lng, mid_lat, mid_lng;
	WaypointQuadtree *nw_child, *ne_child, *sw_child, *se_child;
	std::forward_list<Waypoint*> points;
	unsigned int unique_locations;
	static std::mutex mtx;

	bool refined();
	WaypointQuadtree(double, double, double, double);
	void refine();
	void insert(Waypoint*);
	Waypoint *waypoint_at_same_point(Waypoint*);
	std::forward_list<Waypoint*> near_miss_waypoints(Waypoint*, double);
	std::string str();
	unsigned int size();
	std::forward_list<Waypoint*> point_list();
	std::list<Waypoint*> graph_points();
	bool is_valid(ErrorList &);
	unsigned int max_colocated();
	unsigned int total_nodes();
	void sort();
	void get_tmg_lines(std::list<std::string> &, std::list<std::string> &, std::string);
	void write_qt_tmg(std::string);
};
