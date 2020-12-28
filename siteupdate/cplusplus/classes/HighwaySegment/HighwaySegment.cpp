#include "HighwaySegment.h"
#include "../HighwaySystem/HighwaySystem.h"
#include "../Route/Route.h"
#include "../TravelerList/TravelerList.h"
#include "../Waypoint/Waypoint.h"
#include <cmath>

HighwaySegment::HighwaySegment(Waypoint *w1, Waypoint *w2, Route *rte)
{	waypoint1 = w1;
	waypoint2 = w2;
	route = rte;
	length = waypoint1->distance_to(waypoint2);
	concurrent = 0;
	system_concurrency_count = 1;
	active_only_concurrency_count = 1;
	active_preview_concurrency_count = 1;
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

unsigned int HighwaySegment::index()
{	// segment number:
	// return this segment's index within its route's segment_list vector
	for (unsigned int i = 0; i < route->segment_list.size(); i++)
	  if (route->segment_list[i] == this) return i;
	return -1;	// error; this segment not found in vector
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
			  if (other->clinched_by.find(t) == other->clinched_by.end())
			    return t->traveler_name + " has clinched [" + str() + "], but not [" + other->str() + "]\n";
	  }
	return "";
}//*/

std::string HighwaySegment::clinchedby_code(std::list<TravelerList*> *traveler_lists, unsigned int threadnum)
{	// Return a hexadecimal string encoding which travelers have clinched this segment, for use in "traveled" graph files
	// Each character stores info for traveler #n thru traveler #n+3
	// The first character stores traveler 0 thru traveler 3,
	// The second character stores traveler 4 thru traveler 7, etc.
	// For each character, the low-order bit stores traveler n, and the high bit traveler n+3.

	if (traveler_lists->empty()) return "0";
	std::string code(ceil(double(traveler_lists->size())/4), '0');
	//std::cout << str() << " code string initialized (" << clinched_by.size() << '/' << traveler_lists->size() << ')' << std::endl;
	//unsigned int num = 0;
	for (TravelerList* t : clinched_by)
	{	//std::cout << "\t" << num << ": TravNum = " << t->traveler_num[threadnum] << ": " << t->traveler_name << std::endl;
		//std::cout << "\t" << num << ": TravNum/4 = " << t->traveler_num[threadnum]/4 << std::endl;
		//std::cout << "\t" << num << ": TravNum%4 = " << TravNum%4 << std::endl;
		//std::cout << "\t" << num << ": 2 ^ TravNum%4 = " << pow(2, TravNum%4) << std::endl;
		//std::cout << "\t" << num << ": code[" << TravNum/4 << "] += int(" << pow(2, TravNum%4) << ")" << std::endl;
		code[t->traveler_num[threadnum]/4] += int(pow(2, t->traveler_num[threadnum]%4));
		//num++;
	}
	//std::cout << "travelers written to array" << std::endl;
	for (char &nibble : code) if (nibble > '9') nibble += 7;
	//std::cout << "nibbles >9 -> letters" << std::endl;
	return code;
}

#include "compute_stats_t.cpp"
