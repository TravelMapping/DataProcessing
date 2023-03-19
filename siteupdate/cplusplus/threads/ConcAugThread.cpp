void ConcAugThread(unsigned int id, std::mutex* tl_mtx, std::vector<std::string>* augment_list)
{	//printf("Starting ConcAugThread %02i\n", id); fflush(stdout);
	while (TravelerList::tl_it != TravelerList::allusers.end())
	{	tl_mtx->lock();
		if (TravelerList::tl_it == TravelerList::allusers.end())
		{	tl_mtx->unlock();
			return;
		}
		TravelerList* t(*TravelerList::tl_it);
		//printf("ConcAugThread %02i assigned %s\n", id, t->traveler_name.data()); fflush(stdout);
		TravelerList::tl_it++;
		//printf("ConcAugThread %02i TravelerList::tl_it++\n", id); fflush(stdout);
		tl_mtx->unlock();
		std::cout << '.' << std::flush;
		for (HighwaySegment *s : t->clinched_segments)
		  if (s->concurrent)
		    for (HighwaySegment *hs : *(s->concurrent))
		      if (hs != s && hs->route->system->active_or_preview() && hs->add_clinched_by(t))
		      {	augment_list->push_back("Concurrency augment for traveler " + t->traveler_name + ": [" + hs->str() + "] based on [" + s->str() + ']');
			// create key/value pairs in regional tables, to be computed in a threadsafe manner later
			t->active_preview_mileage_by_region[hs->route->region];
			if (hs->route->system->active())
				t->active_only_mileage_by_region[hs->route->region];
			t->system_region_mileages[hs->route->system][hs->route->region];
		      }
	}
}
