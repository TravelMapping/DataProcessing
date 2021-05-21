class ElapsedTime;
class GraphListEntry;
class HGEdge;
class HGVertex;
class HighwaySystem;
class TravelerList;
class Waypoint;
class WaypointQuadtree;
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
    and determine edges, at most one per concurrent segment.
    Create three sets of edges:
     - one for the simple graph
     - one for the collapsed graph with hidden waypoints compressed into multi-point edges
     - one for the traveled graph: collapsed edges split at endpoints of users' travels
    */

	public:
	std::unordered_set<std::string> vertex_names[256];	// unique vertex labels
	std::list<std::string> waypoint_naming_log;		// to track waypoint name compressions
	std::vector<HGVertex*> vertices;
	std::mutex set_mtx[256], log_mtx;

	HighwayGraph(WaypointQuadtree&, ElapsedTime&);

	void clear();
	void namelog(std::string&&);
	void simplify(int, std::vector<Waypoint*>*, unsigned int*, std::vector<HGVertex*>*);
	inline std::pair<std::unordered_set<std::string>::iterator,bool> vertex_name(std::string&);

	inline void matching_vertices_and_edges
	(	GraphListEntry&, WaypointQuadtree*,
		std::list<TravelerList*> &,
		std::unordered_set<HGVertex*>&,	// final set of vertices matching all criteria
		std::list<HGEdge*>&,		// matching    simple edges
		std::list<HGEdge*>&,		// matching collapsed edges
		std::list<HGEdge*>&,		// matching  traveled edges
		int, unsigned int&, unsigned int&
	);

	void write_master_graphs_tmg();
	void write_subgraphs_tmg(size_t, unsigned int, WaypointQuadtree*, ElapsedTime*, std::mutex*);
};
