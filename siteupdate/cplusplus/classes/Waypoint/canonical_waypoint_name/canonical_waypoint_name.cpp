#include "../Waypoint.h"
#include "../../GraphGeneration/HighwayGraph.h"
#include "../../Route/Route.h"
#include "../../../templates/contains.cpp"
#include <cstring>

std::string Waypoint::canonical_waypoint_name(HighwayGraph* g)
{	/* Best name we can come up with for this point bringing in
	information from itself and colocated points (if active/preview)
	*/
	// start with the failsafe name, and see if we can improve before
	// returning
	std::string name = simple_waypoint_name();

	// if no colocated active/preview points, there's nothing to do - we
	// just return the simple name and deal with conflicts elsewhere
	if (ap_coloc.size() == 1) return name;

	#include "straightforward_intersection.cpp"
	#include "straightforward_concurrency.cpp"
	#include "exit-intersection.cpp"
	#include "exit_number.cpp"
	#include "3plus_intersection.cpp"
	#include "reversed_border_labels.cpp"

	g->namelog("Keep_failsafe: " + name);
	return name;
}
