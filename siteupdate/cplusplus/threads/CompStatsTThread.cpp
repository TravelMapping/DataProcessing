void CompStatsTThread(unsigned int id, std::mutex* list_mtx)
{	//printf("Starting CompStatsTThread %02i\n", id); fflush(stdout);
	while (TravelerList::tl_it != TravelerList::allusers.end())
	{	list_mtx->lock();
		if (TravelerList::tl_it == TravelerList::allusers.end())
		{	list_mtx->unlock();
			return;
		}
		TravelerList* t(*TravelerList::tl_it);
		//printf("CompStatsTThread %02i assigned %s\n", id, t->traveler_name.data()); fflush(stdout);
		TravelerList::tl_it++;
		//printf("CompStatsTThread %02i TravelerList::tl_it++\n", id); fflush(stdout);
		std::cout << '.' << std::flush;
		list_mtx->unlock();
		for (HighwaySegment* s : t->clinched_segments)
		  s->compute_stats_t(t);
	}
}
