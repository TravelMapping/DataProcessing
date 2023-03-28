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
		unsigned int system_concurrency_count = 1;
		unsigned int active_only_concurrency_count = 1;
		unsigned int active_preview_concurrency_count = 1;
		unsigned int overall_concurrency_count = 1;
		if (s->concurrent)
		  for (HighwaySegment *other : *s->concurrent)
		    if (other != s)
		      switch (other->route->system->level) // fall-thru is a Good Thing!
		      {	case 'a': active_only_concurrency_count++;
			case 'p': active_preview_concurrency_count++;
			default : overall_concurrency_count++;
				  if (other->route->system == r->system)
				    system_concurrency_count++;
		      }
		// we know how many times this segment will be encountered
		// in both the system and overall/active+preview/active-only
		// routes, so let's add in the appropriate (possibly fractional)
		// mileage to the overall totals and to the system categorized
		// by its region
		switch (r->system->level) // fall-thru is a Good Thing!
		{   case 'a':	 active_only_mileage += s->length/active_only_concurrency_count;
		    case 'p': active_preview_mileage += s->length/active_preview_concurrency_count;
		    default :	      system_mileage += s->length/system_concurrency_count;
				     overall_mileage += s->length/overall_concurrency_count;
		}
		// that's it for overall stats, now credit all travelers
		// who have clinched this segment in their stats
		for (TravelerList *t : s->clinched_by)
		{	// credit active+preview for this region, which it must be
			// if this segment is clinched by anyone
			t->active_preview_mileage_by_region[this] += s->length/active_preview_concurrency_count;

			// credit active only for this region
			if (r->system->active())
				t->active_only_mileage_by_region[this] += s->length/active_only_concurrency_count;

			// credit this system in this region in the messy unordered_map of unordered_maps
			t->system_region_mileages[r->system][this] += s->length/system_concurrency_count;
		}
	}
    }
}
