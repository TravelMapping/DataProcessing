class ConnectedRoute;
class ErrorList;
class HGVertex;
class Region;
class Route;
#include <list>
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

	std::list<Route*> route_list;
	std::list<ConnectedRoute*> con_route_list;
	std::unordered_map<Region*, double> mileage_by_region;
	std::unordered_set<HGVertex*> vertices;
	std::unordered_set<std::string>listnamesinuse, unusedaltroutenames;
	std::mutex lniu_mtx, uarn_mtx;
	bool is_valid;

	static std::list<HighwaySystem*> syslist;
	static std::list<HighwaySystem*>::iterator it;

	HighwaySystem(std::string &, ErrorList &, std::vector<std::pair<std::string,std::string>> &);

	bool active();			// Return whether this is an active system
	bool preview();			// Return whether this is a preview system
	bool active_or_preview();	// Return whether this is an active or preview system
	bool devel();			// Return whether this is a development system
	double total_mileage();		// Return total system mileage across all regions
	std::string level_name();	// return full "active" / "preview" / "devel" string"
};
