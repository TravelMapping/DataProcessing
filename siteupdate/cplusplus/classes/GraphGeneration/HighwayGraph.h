class ElapsedTime;
class GraphListEntry;
class HGEdge;
class HGVertex;
class HighwaySystem;
class TravelerList;
class Waypoint;
class WaypointQuadtree;
#include "../../templates/TMArray.cpp"
#include <list>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>

class HighwayGraph
{   /* This class implements the capability to create graph
    data structures representing the highway data.

    On construction, build a set of unique vertex names
    and determine edges, at most one per concurrent segment per graph format.
    Edge objects can be 1 or more of the following per the HGEdge::format bitmask:
      - in the simple graph
      - in the collapsed graph with hidden waypoints compressed into multi-point edges
      - in the traveled graph: collapsed edges split at endpoints of users' travels
      - in no graph at all for temporary partially-collapsed edges created
	during the compression process. These stick around taking up
	space until the HighwayGraph object is destroyed. Mostly harmless.
    */

	public:
	std::unordered_set<std::string> vertex_names[256];	// unique vertex labels
	std::list<std::string> waypoint_naming_log;		// to track waypoint name compressions
	std::mutex set_mtx[256], log_mtx;
	std::vector<HGVertex> vertices;				// MUST be stored
	TMArray<HGEdge> edges;					// sequentially!
	unsigned int cv, tv, se, ce, te;			// vertex & edge counts

	HighwayGraph(WaypointQuadtree&, ElapsedTime&);

	void namelog(std::string&&);
	void simplify(int, std::vector<std::pair<Waypoint*,size_t>>*, unsigned int*);
	void bitsetlogs(HGVertex*);
	inline std::pair<std::unordered_set<std::string>::iterator,bool> vertex_name(std::string&);
	void write_master_graphs_tmg();
	void write_subgraphs_tmg(size_t, unsigned int, WaypointQuadtree*, ElapsedTime*, std::mutex*);
};
