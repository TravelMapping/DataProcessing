class HighwaySystem;
class PlaceRadius;
class Region;
#include <list>
#include <string>

class GraphListEntry
{	/* This class encapsulates information about generated graphs for
	inclusion in the DB table.  Field names here match column names
	in the "graphs" DB table. */
	public:
	std::string filename();
	std::string descr;
	unsigned int vertices;
	unsigned int edges;
	unsigned int travelers;
	char form;
	std::string format();
	char cat;
	std::string category();
	// additional data for the C++ version, for multithreaded subgraph writing
	std::string root;
	std::list<Region*> *regions;
	std::list<HighwaySystem*> *systems;
	PlaceRadius *placeradius;
	std::string tag();

	GraphListEntry(std::string, std::string, char, char, std::list<Region*>*, std::list<HighwaySystem*>*, PlaceRadius*);
};
