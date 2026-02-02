class ErrorList;
class HighwaySystem;
class PlaceRadius;
class Region;
#include <string>
#include <unordered_map>
#include <vector>

class GraphListEntry
{	/* This class encapsulates information about generated graphs for
	inclusion in the DB table.  Field names here
	(or function names if both are on the same line)
	match column names in the "graphs" DB table. */
	public:
	// Info for use in threaded subgraph generation. Not used in DB.
	std::vector<Region*> *regions;
	std::vector<HighwaySystem*> *systems;
	PlaceRadius *placeradius;

	// Info for the "graphs" DB table
	std::string root;	std::string filename();
	std::string descr;
	unsigned int vertices;
	unsigned int edges;
	unsigned int travelers;
	char form;		std::string format();
	char cat;		std::string category();

	static std::vector<GraphListEntry> entries;
	static size_t num; // iterator for entries
	static std::unordered_map<std::string, GraphListEntry*> unique_names;

	GraphListEntry(std::string, std::string, char, char, std::vector<Region*>*, std::vector<HighwaySystem*>*, PlaceRadius*);
	static void add_group(std::string&&,  std::string&&,  char, std::vector<Region*>*, std::vector<HighwaySystem*>*, PlaceRadius*, ErrorList&);
	std::string tag();
};
