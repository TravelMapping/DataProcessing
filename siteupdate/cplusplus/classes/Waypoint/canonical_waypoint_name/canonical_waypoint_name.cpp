std::string Waypoint::canonical_waypoint_name(std::list<std::string> &log, std::unordered_set<std::string> &vertex_names, DatacheckEntryList *datacheckerrors)
{	/* Best name we can come up with for this point bringing in
	information from itself and colocated points (if active/preview)
	*/
	// start with the failsafe name, and see if we can improve before
	// returning
	std::string name = simple_waypoint_name();

	// if no colocated points, there's nothing to do - we just use
	// the route@label form and deal with conflicts elsewhere
	if (!colocated) return name;

	// just return the simple name if only one active/preview waypoint
	if (ap_coloc.size() == 1) return name;

	#include "straightforward_intersection.cpp"
	#include "straightforward_concurrency.cpp"
	#include "exit-intersection.cpp"
	#include "exit_number.cpp"
	#include "3plus_intersection.cpp"
	#include "reversed_border_labels.cpp"

	// TODO: I-20@76&I-77@16
	// should become I-20/I-77 or maybe I-20(76)/I-77(16)
	// not shorter, so maybe who cares about this one?

	// TODO: US83@FM1263_S&US380@FM1263
	// should probably end up as US83/US280@FM1263 or @FM1263_S

	// How about?
	// I-581@4&US220@I-581(4)&US460@I-581&US11AltRoa@I-581&US220AltRoa@US220_S&VA116@I-581(4)

	// TODO: I-610@TX288&I-610@38&TX288@I-610
	// this is the overlap point of a loop

	log.push_back("Keep_failsafe: " + name);
	return name;
}
