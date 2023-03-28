#include "HighwaySegment.h"
#include "../HighwaySystem/HighwaySystem.h"
#include "../Route/Route.h"
#include "../TravelerList/TravelerList.h"
#include "../Waypoint/Waypoint.h"
#include "../../templates/contains.cpp"

HighwaySegment::HighwaySegment(Waypoint *w1, Waypoint *w2, Route *rte)
{	waypoint1 = w1;
	waypoint2 = w2;
	route = rte;
	length = waypoint1->distance_to(waypoint2);
	concurrent = 0;
}

std::string HighwaySegment::str()
{	return route->readable_name() + " " + waypoint1->label + " " + waypoint2->label;
}

bool HighwaySegment::add_clinched_by(TravelerList *traveler)
{	clin_mtx.lock();
	bool result = clinched_by.insert(traveler).second;
	clin_mtx.unlock();
	return result;
}

std::string HighwaySegment::csv_line(unsigned int id)
{	/* return csv line to insert into a table */
	return "'" + std::to_string(id) + "','" + std::to_string(waypoint1->point_num) + "','" + std::to_string(waypoint2->point_num) + "','" + route->root + "'";
}

std::string HighwaySegment::segment_name()
{	/* compute a segment name based on names of all
	concurrent routes, used for graph edge labels */
	std::string segment_name;
	if (!concurrent)
	{	if (route->system->active_or_preview())
		  segment_name = route->list_entry_name();
	} else
	  for (HighwaySegment *cs : *concurrent)
	    if (cs->route->system->active_or_preview())
	    {	if (segment_name != "") segment_name += ",";
		segment_name += cs->route->list_entry_name();
	    }
	return segment_name;
}

/*std::string HighwaySegment::concurrent_travelers_sanity_check()
{	if (route->system->devel()) return "";
	if (concurrent)
	  for (HighwaySegment *other : *concurrent)
	  {	if (clinched_by.size() != other->clinched_by.size())
		{	if (other->route->system->devel()) continue;
			return "[" + str() + "] clinched by " + std::to_string(clinched_by.size()) + " travelers; [" \
			     + other->str() + "] clinched by " + std::to_string(other->clinched_by.size()) + '\n';
		}
		else	for (TravelerList *t : clinched_by)
			  if (other->clinched_by.count(t))
			    return t->traveler_name + " has clinched [" + str() + "], but not [" + other->str() + "]\n";
	  }
	return "";
}//*/

const char* HighwaySegment::clinchedby_code(std::list<TravelerList*> *traveler_lists, char* code, unsigned int threadnum)
{	// Compute a hexadecimal string encoding which travelers
	// have clinched this segment, for use in "traveled" graph files.
	// Each character stores info for traveler #n thru traveler #n+3.
	// The first character stores traveler 0 thru traveler 3,
	// The second character stores traveler 4 thru traveler 7, etc.
	// For each character, the low-order bit stores traveler n, and the high bit traveler n+3.

	if (traveler_lists->empty()) return "0";
	for (TravelerList* t : clinched_by)
		code[t->traveler_num[threadnum]/4] += 1 << t->traveler_num[threadnum]%4;
	for (char* nibble = code; *nibble; ++nibble) if (*nibble > '9') *nibble += 7;
	return code;
}

bool HighwaySegment::system_match(std::list<HighwaySystem*>* systems)
{	if (!systems) return 1;
	// devel routes are already excluded from graphs,
	// so no need to check on non-concurrent segments
	if (!concurrent) return contains(*systems, route->system);
	for (HighwaySegment *cs : *(concurrent))
	  // no devel systems should be listed
	  // in CSVs, so no need for that check
	  if (contains(*systems, cs->route->system)) return 1;
	return 0;
}

// compute an edge label, optionally restricted by systems
void HighwaySegment::write_label(std::ofstream& file, std::list<HighwaySystem*> *systems)
{	if (concurrent)
	     {	bool write_comma = 0;
		for (HighwaySegment* cs : *concurrent)
		  if ( !cs->route->system->devel() && (!systems || contains(*systems, cs->route->system)) )
		  {	if  (write_comma) file << ',';
			else write_comma = 1;
			file << cs->route->route;
			file << cs->route->banner;
			file << cs->route->abbrev;
		  }
	     }
	else {	file << route->route;
		file << route->banner;
		file << route->abbrev;
	     }
}
