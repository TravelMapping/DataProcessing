#include <list>
#include <mutex>
#include <string>
#include <vector>

class ClinchedDBValues
{	public:
	// this will be used to store DB entry lines...
	std::list<std::string> csmbr_values;	// for clinchedSystemMileageByRegion table
	std::vector<std::string> ccr_values;	// and similar for DB entry lines for clinchedConnectedRoutes table
	std::vector<std::string> cr_values;	// and clinchedRoutes table
	// ...to be added into the DB later in the program
	std::mutex csmbr_mtx, ccr_mtx, cr_mtx;

	void add_csmbr(std::string);
	void add_ccr(std::string);
	void add_cr(std::string);
};
