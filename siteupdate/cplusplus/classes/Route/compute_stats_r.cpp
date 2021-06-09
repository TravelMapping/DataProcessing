void Route::compute_stats_r()
{	double active_only_mileage = 0;
	double active_preview_mileage = 0;
	double overall_mileage = 0;
	double system_mileage = 0;
	for (HighwaySegment *s : segment_list)
	{	// always add the segment mileage to the route
		mileage += s->length;
		//#include "../debug/RtMiSegLen.cpp"

		// but we do need to check for concurrencies for others
		unsigned int overall_concurrency_count = 1;
		if (s->concurrent)
		  for (HighwaySegment *other : *s->concurrent)
		    if (other != s)
		    {	overall_concurrency_count++;
			if (other->route->system->active_or_preview())
			{	s->active_preview_concurrency_count++;
				if (other->route->system->active()) s->active_only_concurrency_count++;
			}
			if (other->route->system == system) s->system_concurrency_count++;
		    }
		// we know how many times this segment will be encountered
		// in both the system and overall/active+preview/active-only
		// routes, so let's add in the appropriate (possibly fractional)
		// mileage to the overall totals and to the system categorized
		// by its region

		// first, add to overall mileage for this region
		overall_mileage += s->length/overall_concurrency_count;

		// next, same thing for active_preview mileage for the region,
		// if active or preview
		if (system->active_or_preview())
		  active_preview_mileage += s->length/s->active_preview_concurrency_count;

		// now same thing for active_only mileage for the region,
		// if active
		if (system->active())
		  active_only_mileage += s->length/s->active_only_concurrency_count;

		// now we move on to totals by region, only the
		// overall since an entire highway system must be
		// at the same level
		system_mileage += s->length/s->system_concurrency_count;
	}
	// now that we have the subtotal for this route, add it to the regional totals
	region->mtx.lock();
	region->overall_mileage += overall_mileage;
	region->active_preview_mileage += active_preview_mileage;
	region->active_only_mileage += active_only_mileage;
	region->mtx.unlock();
	try {	system->mileage_by_region.at(region) += system_mileage;
	    }
	catch (const std::out_of_range& oor)
	    {	system->mileage_by_region[region] = system_mileage;
	    }

	// datachecks
	if (abbrev.empty())
	     {	if ( banner.size() && !strncmp(banner.data(), city.data(), banner.size()) )
		  Datacheck::add(this, "", "", "", "ABBREV_AS_CHOP_BANNER",
				 system->systemname + ".csv#L" +
				 std::to_string(system->route_index(this)+2));
	     }
}
