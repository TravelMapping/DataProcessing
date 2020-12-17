void HighwaySegment::compute_stats_t(TravelerList* t)
{	// credit active+preview for this region, which it must be
	// if this segment is clinched by anyone
	try {	t->active_preview_mileage_by_region.at(route->region) += length/active_preview_concurrency_count;
	    }
	catch (const std::out_of_range& oor)
	    {	t->active_preview_mileage_by_region[route->region] = length/active_preview_concurrency_count;
	    }

	// credit active only for this region
	if (route->system->active())
	{	try {	t->active_only_mileage_by_region.at(route->region) += length/active_only_concurrency_count;
		    }
		catch (const std::out_of_range& oor)
		    {	t->active_only_mileage_by_region[route->region] = length/active_only_concurrency_count;
		    }
	}

	// credit this system in this region in the messy unordered_map of unordered_maps
	try {	t->system_region_mileages[route->system].at(route->region) += length/system_concurrency_count;
	    }
	catch (const std::out_of_range& oor)
	    {	t->system_region_mileages[route->system][route->region] = length/system_concurrency_count;
	    }
}
