class HighwaySystem;
class PlaceRadius;
class Region;
#include <list>
#include <string>
#include <vector>

class GraphListEntry
{	/* This class encapsulates information about generated graphs for
	inclusion in the DB table.  Field names here
	(or function names if both are on the same line)
	match column names in the "graphs" DB table. */
	public:
	std::string root;	std::string filename();
	std::string descr;
	std::list<Region*> *regions;		// C++ only, not in DB or Python
	std::list<HighwaySystem*> *systems;	// C++ only, not in DB or Python
	PlaceRadius *placeradius;		// C++ only, not in DB or Python
	unsigned int vertices;
	unsigned int edges;
	unsigned int travelers;
	char form;		std::string format();
	char cat;		std::string category();

	static std::vector<GraphListEntry> entries;
	static size_t num; // iterator for entries
	std::string tag();

	GraphListEntry(std::string, std::string, char, char, std::list<Region*>*, std::list<HighwaySystem*>*, PlaceRadius*);
};
