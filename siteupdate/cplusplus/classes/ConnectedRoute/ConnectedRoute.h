class ErrorList;
class HighwaySystem;
class Route;
#include <string>
#include <vector>

class ConnectedRoute
{   /* This class encapsulates a single 'connected route' as given
    by a single line of a _con.csv file
    */

	public:
	HighwaySystem *system;
	std::string route;
	std::string banner;
	std::string groupname;
	std::vector<Route*> roots;
	double mileage;		// will be computed for routes in active & preview systems
	bool disconnected;	// whether any DISCONNECTED_ROUTE errors are flagged for this ConnectedRoute

	ConnectedRoute(std::string &, HighwaySystem *, ErrorList &);

	std::string connected_rtes_line();
	std::string readable_name();
	size_t index();
	//std::string list_lines(int, int, std::string, size_t);

	// datacheck
	void verify_connectivity();
	void combine_con_routes();
};
