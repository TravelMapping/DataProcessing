#include "Route.h"
#include "../HighwaySegment/HighwaySegment.h"
#include "../HighwaySystem/HighwaySystem.h"
#include "../TravelerList/TravelerList.h"
#include <fstream>

void Route::store_traveled_segments(TravelerList* t, std::ofstream& log, std::string& update, unsigned int beg, unsigned int end)
{	// store clinched segments with traveler and traveler with segments
	size_t index = t-TravelerList::allusers.data;
	for (unsigned int pos = beg; pos < end; pos++)
	{	HighwaySegment *hs = segments.data+pos;
		if (hs->add_clinched_by(index))
		  t->clinched_segments.push_back(hs);
	}
      #ifdef threading_enabled
	// create key/value pairs in regional tables, to be computed in a threadsafe manner later
	if (system->active())
	   t->active_only_mileage_by_region[region];
	t->active_preview_mileage_by_region[region];
	t->system_region_mileages[system][region];
      #endif
	// userlog notification for routes updated more recently than .list file
	if (last_update && t->updated_routes.insert(this).second && update.size() && last_update[0] >= update)
		log << "Route updated " << last_update[0] << ": " << readable_name() << '\n';
}
