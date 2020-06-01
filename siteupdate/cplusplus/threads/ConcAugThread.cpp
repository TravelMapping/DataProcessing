void ConcAugThread(unsigned int id, std::list<TravelerList*> *travlists, std::list<TravelerList*>::iterator *it,
		   std::mutex *tl_mtx, std::list<std::string>* augment_list)
{	//printf("Starting ConcAugThread %02i\n", id); fflush(stdout);
	while (*it != travlists->end())
	{	tl_mtx->lock();
		if (*it == travlists->end())
		{	tl_mtx->unlock();
			return;
		}
		TravelerList *t(**it);
		//printf("ConcAugThread %02i assigned %s\n", id, t->traveler_name.data()); fflush(stdout);
		(*it)++;
		//printf("ConcAugThread %02i (*it)++\n", id); fflush(stdout);
		tl_mtx->unlock();
		std::cout << '.' << std::flush;
		for (HighwaySegment *s : t->clinched_segments)
		  if (s->concurrent)
		    for (HighwaySegment *hs : *(s->concurrent))
		      if (hs != s && hs->route->system->active_or_preview() && hs->add_clinched_by(t))
			augment_list->push_back("Concurrency augment for traveler " + t->traveler_name + ": [" + hs->str() + "] based on [" + s->str() + ']');
	}
}
