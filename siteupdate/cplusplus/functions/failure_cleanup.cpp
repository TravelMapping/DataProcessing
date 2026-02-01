#include "../classes/Args/Args.h"
#include "../classes/GraphGeneration/GraphListEntry.h"
#include "../classes/GraphGeneration/PlaceRadius.h"
#include "../classes/WaypointQuadtree/WaypointQuadtree.h"
#include <vector>

void failure_cleanup
(	WaypointQuadtree& all_waypoints, std::vector<unsigned int>& colocate_counts,
	std::list<std::string*>& updates, std::list<std::string*>& systemupdates
)
{	Args::colocationlimit = 0;			// silence colocation reports
	all_waypoints.final_report(colocate_counts);	// destroy quadtree nodes & colocation lists
	for (auto g = GraphListEntry::entries.begin(); g < GraphListEntry::entries.end(); g += 3)
	{	delete g->regions;			// destroy
		delete g->systems;			// GraphListEntry
		delete g->placeradius;			// data
	}
	for (std::string* u : updates)		delete[] u; // destroy updates
	for (std::string* u : systemupdates)	delete[] u; // & systemupdates
}
