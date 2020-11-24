class TravelerList
{   /* This class encapsulates the contents of one .list file
    that represents the travels of one individual user.

    A list file consists of lines of 4 values:
    region route_name start_waypoint end_waypoint

    which indicates that the user has traveled the highway names
    route_name in the given region between the waypoints named
    start_waypoint end_waypoint
    */
	public:
	std::mutex ap_mi_mtx, ao_mi_mtx, sr_mi_mtx;
	std::unordered_set<HighwaySegment*> clinched_segments;
	std::string traveler_name;
	std::unordered_map<Region*, double> active_preview_mileage_by_region;				// total mileage per region, active+preview only
	std::unordered_map<Region*, double> active_only_mileage_by_region;				// total mileage per region, active only
	std::unordered_map<HighwaySystem*, std::unordered_map<Region*, double>> system_region_mileages;	// mileage per region per system
	std::unordered_set<Route*> routes;
	unsigned int *traveler_num;
	unsigned int active_systems_traveled;
	unsigned int active_systems_clinched;
	unsigned int preview_systems_traveled;
	unsigned int preview_systems_clinched;
	// for locking the traveler_lists list when reading .lists from disk
	// and avoiding data races when creating userlog timestamps
	static std::mutex alltrav_mtx;

	TravelerList(std::string, ErrorList *, Arguments *);
	double active_only_miles();
	double active_preview_miles();
	double system_region_miles(HighwaySystem *);
	void userlog(ClinchedDBValues *, const double, const double, std::list<HighwaySystem*>*, std::string path);
};
