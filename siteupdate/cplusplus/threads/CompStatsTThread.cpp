void CompStatsTThread
(	unsigned int id, std::list<TravelerList*> *traveler_lists,
	std::list<TravelerList*>::iterator *it, std::mutex *list_mtx
)
{	//printf("Starting CompStatsTThread %02i\n", id); fflush(stdout);
	while (*it != traveler_lists->end())
	{	list_mtx->lock();
		if (*it == traveler_lists->end())
		{	list_mtx->unlock();
			return;
		}
		TravelerList* t(**it);
		//printf("CompStatsTThread %02i assigned %s\n", id, t->traveler_name.data()); fflush(stdout);
		(*it)++;
		//printf("CompStatsTThread %02i (*it)++\n", id); fflush(stdout);
		std::cout << '.' << std::flush;
		list_mtx->unlock();
		for (HighwaySegment* s : t->clinched_segments)
		  s->compute_stats_t(t);
	}
}
