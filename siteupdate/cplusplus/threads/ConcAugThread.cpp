void ConcAugThread(unsigned int id, std::mutex* tl_mtx, std::list<std::string>* augment_list)
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
		std::list<HighwaySegment*> to_add;
		for (HighwaySegment *s : t->clinched_segments)
		  if (s->concurrent)
		    for (HighwaySegment *hs : *(s->concurrent))
		      if (hs != s && hs->route->system->active_or_preview() && hs->add_clinched_by(t))
		      {	augment_list->push_back("Concurrency augment for traveler " + t->traveler_name + ": [" + hs->str() + "] based on [" + s->str() + ']');
			to_add.push_back(hs);
		      }
		t->clinched_segments.insert(to_add.begin(), to_add.end());
	}
}
