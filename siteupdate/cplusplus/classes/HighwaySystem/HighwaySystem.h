class ConnectedRoute;
class ErrorList;
class HGVertex;
class Region;
class Route;
#include "../../templates/TMArray.cpp"
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class HighwaySystem
{	/* This class encapsulates the contents of one .csv file
	that represents the collection of highways within a system.

	See Route for information about the fields of a .csv file

	With the implementation of three levels of systems (active,
	preview, devel), added parameter and field here, to be stored in
	DB

	After construction and when all Route entries are made, a _con.csv
	file is read that defines the connected routes in the system.
	In most cases, the connected route is just a single Route, but when
	a designation within the same system crosses region boundaries,
	a connected route defines the entirety of the route.
	*/

	public:
	std::string systemname;
	std::pair<std::string, std::string> *country;
	std::string fullname;
	std::string color;
	short tier;
	char level; // 'a' for active, 'p' for preview, 'd' for devel

	TMArray<Route> routes;
	TMArray<ConnectedRoute> con_routes;
      #ifdef Ubuntu_16_04_7_LTS
	// If this padding is omitted, compute_stats runs about 76% as fast,
	// (69% on siteupdateST) with both clang and gcc. ~74-82% the speed
	// of 79fbeb3 *before* the TMArray conversion. It's not clear why --
	// while this does pad objects out to 384 bytes, exactly 6 cache lines,
	// 1. No attempt to over-align the data has been made.
	// 2. Dropping below 384 B hurts performance, but not going above.
	// Nonetheless, the benchmarks are clear & consistent.
	size_t padding;
      #endif
	std::vector<HGVertex*> vertices;
	std::unordered_map<Region*, double> mileage_by_region;
	std::unordered_set<std::string>listnamesinuse, unusedaltroutenames;
	std::mutex mtx;

	static TMArray<HighwaySystem> syslist;
	static HighwaySystem* it;
	static unsigned int num_active;
	static unsigned int num_preview;

	HighwaySystem(std::string &, ErrorList &);

	bool active();			// Return whether this is an active system
	bool preview();			// Return whether this is a preview system
	bool active_or_preview();	// Return whether this is an active or preview system
	bool devel();			// Return whether this is a development system
	double total_mileage();		// Return total system mileage across all regions
	std::string level_name();	// Return full "active" / "preview" / "devel" string
	void route_integrity(ErrorList& el);
	void add_vertex(HGVertex*);
	void stats_csv();
	void mark_route_in_use(std::string&);
	void mark_routes_in_use(std::string&, std::string&);

	static void systems_csv(ErrorList&);
};
