#include "Region.h"
#include "../HighwaySegment/HighwaySegment.h"
#include "../HighwaySystem/HighwaySystem.h"
#include "../Route/Route.h"
#include "../TravelerList/TravelerList.h"
#include <iostream>

void Region::compute_stats()
{   std::cout << '.' << std::flush;
    for (Route* const r : routes)
    {	double& system_mileage = r->system->mileage_by_region[this];
	for (HighwaySegment *s : r->segment_list)
	{	// always add the segment mileage to the route
		r->mileage += s->length;

		// but we do need to check for concurrencies for others
		unsigned int sys_concurrency_count = 1; // system
		unsigned int act_concurrency_count = 1; // active only
		unsigned int a_p_concurrency_count = 1; // active or preview
		unsigned int all_concurrency_count = 1; // overall
		if (s->concurrent)
		  for (HighwaySegment *other : *s->concurrent)
		    if (other != s)
		      switch (other->route->system->level) // fall-thru is a Good Thing!
		      {	case 'a': ++act_concurrency_count;
			case 'p': ++a_p_concurrency_count;
			default : ++all_concurrency_count;
				  if (other->route->system == r->system)
				    sys_concurrency_count++;
		      }
		// we know how many times this segment will be encountered
		// in both the system and overall/active+preview/active-only
		// routes, so let's add in the appropriate (possibly fractional)
		// mileage to the overall totals and to the system categorized
		// by its region
		switch (r->system->level) // fall-thru is a Good Thing!
		{   case 'a':	active_only_mileage    += s->length/act_concurrency_count;
		    case 'p':	active_preview_mileage += s->length/a_p_concurrency_count;
				// credit all travelers who've clinched this segment in their stats
				for (TravelerList *t : s->clinched_by)
				{	if (r->system->active())
					   t->active_only_mileage_by_region[this]  += s->length/act_concurrency_count;
					t->active_preview_mileage_by_region[this]  += s->length/a_p_concurrency_count;
					t->system_region_mileages[r->system][this] += s->length/sys_concurrency_count;
				}
		    default :	system_mileage  += s->length/sys_concurrency_count;
				overall_mileage += s->length/all_concurrency_count;
		}
	}
    }
}
