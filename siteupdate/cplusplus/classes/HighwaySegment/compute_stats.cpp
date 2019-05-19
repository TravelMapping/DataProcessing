void HighwaySegment::compute_stats()
{	// always add the segment mileage to the route
	route->mileage += length;
	//#include "../debug/RtMiSegLen.cpp"

	// but we do need to check for concurrencies for others
	unsigned int system_concurrency_count = 1;
	unsigned int active_only_concurrency_count = 1;
	unsigned int active_preview_concurrency_count = 1;
	unsigned int overall_concurrency_count = 1;
	if (concurrent)
	  for (HighwaySegment *other : *concurrent)
	    if (other != this)
	    {	overall_concurrency_count++;
		if (other->route->system->active_or_preview())
		{	active_preview_concurrency_count++;
			if (other->route->system->active()) active_only_concurrency_count++;
		}
		if (other->route->system == route->system) system_concurrency_count++;
	    }
	// we know how many times this segment will be encountered
	// in both the system and overall/active+preview/active-only
	// routes, so let's add in the appropriate (possibly fractional)
	// mileage to the overall totals and to the system categorized
	// by its region

	// first, add to overall mileage for this region
	route->region->ov_mi_mtx->lock();
	route->region->overall_mileage += length/overall_concurrency_count;
	route->region->ov_mi_mtx->unlock();

	// next, same thing for active_preview mileage for the region,
	// if active or preview
	if (route->system->active_or_preview())
	{	route->region->ap_mi_mtx->lock();
		route->region->active_preview_mileage += length/active_preview_concurrency_count;
		route->region->ap_mi_mtx->unlock();
	}

	// now same thing for active_only mileage for the region,
	// if active
	if (route->system->active())
	{	route->region->ao_mi_mtx->lock();
		route->region->active_only_mileage += length/active_only_concurrency_count;
		route->region->ao_mi_mtx->unlock();
	}

	// now we move on to totals by region, only the
	// overall since an entire highway system must be
	// at the same level
	try {	route->system->mileage_by_region.at(route->region) += length/system_concurrency_count;
	    }
	catch (const std::out_of_range& oor)
	    {	route->system->mileage_by_region[route->region] = length/system_concurrency_count;
	    }

	// that's it for overall stats, now credit all travelers
	// who have clinched this segment in their stats
	for (TravelerList *t : clinched_by)
	{	// credit active+preview for this region, which it must be
		// if this segment is clinched by anyone but still check
		// in case a concurrency detection might otherwise credit
		// a traveler with miles in a devel system
		if (route->system->active_or_preview())
		{	t->ap_mi_mtx.lock();
			try {	t->active_preview_mileage_by_region.at(route->region) += length/active_preview_concurrency_count;
			    }
			catch (const std::out_of_range& oor)
			    {	t->active_preview_mileage_by_region[route->region] = length/active_preview_concurrency_count;
			    }
			t->ap_mi_mtx.unlock();
		}

		// credit active only for this region
		if (route->system->active())
		{	t->ao_mi_mtx.lock();
			try {	t->active_only_mileage_by_region.at(route->region) += length/active_only_concurrency_count;
			    }
			catch (const std::out_of_range& oor)
			    {	t->active_only_mileage_by_region[route->region] = length/active_only_concurrency_count;
			    }
			t->ao_mi_mtx.unlock();
		}

		// credit this system in this region in the messy unordered_map
		// of unordered_maps, but skip devel system entries
		if (route->system->active_or_preview())
		{	t->sr_mi_mtx.lock();
			try {	t->system_region_mileages[route->system].at(route->region) += length/system_concurrency_count;
			    }
			catch (const std::out_of_range& oor)
			    {	t->system_region_mileages[route->system][route->region] = length/system_concurrency_count;
			    }
			t->sr_mi_mtx.unlock();
		}
	}
}
