#include "HighwaySegment.h"
#include "../HighwaySystem/HighwaySystem.h"
#include "../Route/Route.h"
#include "../TravelerList/TravelerList.h"
#include "../Waypoint/Waypoint.h"
#include "../../templates/contains.cpp"

HighwaySegment::HighwaySegment(Waypoint *w, Route *rte):
	waypoint1(w-1),
	waypoint2(w),
	route(rte),
	length(waypoint1->distance_to(w)),
	concurrent(0),
	clinched_by(TravelerList::allusers.data, TravelerList::allusers.size) {}

HighwaySegment::~HighwaySegment()
{	if (concurrent)
	{	auto conc = concurrent;
		for (HighwaySegment* cs : *conc)
			cs->concurrent = 0;
		delete conc;
	}
}

std::string HighwaySegment::str()
{	return route->readable_name() + " " + waypoint1->label + " " + waypoint2->label;
}

void HighwaySegment::add_concurrency(std::ofstream& concurrencyfile, Waypoint* w)
{	HighwaySegment& other = w->route->segments[w - w->route->points.data];
	if (!concurrent)
	     {	concurrent = new std::list<HighwaySegment*>;
			     // deleted by ~HighwaySegment
		concurrent->push_back(this);
		concurrent->push_back(&other);
		concurrencyfile << "New concurrency [" << str() << "][" << other.str() << "] (2)\n";
	     }
	else {	concurrent->push_back(&other);
		concurrencyfile << "Extended concurrency ";
		for (HighwaySegment *x : *concurrent)
			concurrencyfile << '[' << x->str() << ']';
		concurrencyfile << " (" << concurrent->size() << ")\n";
	     }
	other.concurrent = concurrent;
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

const char* HighwaySegment::clinchedby_code(char* code, unsigned int threadnum)
{	// Compute a hexadecimal string encoding which travelers
	// have clinched this segment, for use in "traveled" graph files.
	// Each character stores info for traveler #n thru traveler #n+3.
	// The first character stores traveler 0 thru traveler 3,
	// The second character stores traveler 4 thru traveler 7, etc.
	// For each character, the low-order bit stores traveler n, and the high bit traveler n+3.

	for (TravelerList* t : clinched_by)
		code[t->traveler_num[threadnum]/4] += 1 << t->traveler_num[threadnum]%4;
	for (char* nibble = code; *nibble; ++nibble) if (*nibble > '9') *nibble += 7;
	return code;
}

// compute an edge label, optionally restricted by systems
void HighwaySegment::write_label(std::ofstream& file, std::vector<HighwaySystem*> *systems)
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

// find canonical segment for HGEdge construction, just in case a devel system is listed earlier in systems.csv
HighwaySegment* HighwaySegment::canonical_edge_segment()
{	if (!concurrent) return this;
	for (auto& s : *concurrent)
	  if (s->route->system->active_or_preview())
	    return s;
	// We should never reach this point, because this function will
	// only be called on active/preview segments and a concurrent
	// segment should always be in its own concurrency list.
	// Nonetheless, let's stop the compiler from complaining.
	throw 0xBAD;
}
