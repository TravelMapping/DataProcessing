class ErrorList;
class HGEdge;
class HGVertex;
class Route;
class Waypoint;
#include "../../templates/TMArray.cpp"
#include "../../templates/TMBitset.cpp"
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

std::pair<std::string, std::string> *country_or_continent_by_code(std::string, std::vector<std::pair<std::string, std::string>>&);

class Region
{   /* This class encapsulates the contents of one .csv file line
    representing a region in regions.csv

    The format of the .csv file for a region is a set of
    semicolon-separated lines, each of which has 5 fields:

    code;name;country;continent;regionType

    The first line names these fields, subsequent lines exist,
    one per region, with values for each field.

    code: the region code, as used in .list files, or HB or stats
    queries, etc.
    E.G. AB, ABW, AUS-ACT, etc.

    name: the full name of the region
    E.G. Alberta, Aruba, Australian Capital Territory, etc.

    country: the code for the country in which the region is located
    E.G. CAN, NLD, AUS, etc.

    continent: the code for the continent on which the region is located
    E.G. NA, OCE, ASI, etc.

    regionType: a description of the region type
    E.G. Province, Constituent Country, Territory, etc.
    */

	public:
	std::pair<std::string, std::string> *country, *continent;
	std::string code, name, type;
	double active_only_mileage;
	double active_preview_mileage;
	double overall_mileage;
	std::vector<Route*> routes;
	TMBitset<HGVertex*, uint64_t> vertices;
	TMBitset<HGEdge*,   uint64_t> edges;

	static TMArray<Region> allregions;
	static Region* it;
	static std::unordered_map<std::string, Region*> code_hash;
	static std::vector<std::pair<std::string, std::string>> continents;
	static std::vector<std::pair<std::string, std::string>> countries;

	Region (const std::string&, ErrorList&);

	void compute_stats();
	std::string &country_code();
	std::string &continent_code();
	static void read_csvs(ErrorList&);
	static void cccsv(ErrorList&, std::string, std::string, size_t, size_t, std::vector<std::pair<std::string, std::string>>&);
	static void ve_thread(std::mutex* mtx, std::vector<HGVertex>*, TMArray<HGEdge>*);
};
