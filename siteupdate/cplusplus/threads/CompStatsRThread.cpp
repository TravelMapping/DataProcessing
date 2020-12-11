void CompStatsRThread(unsigned int id, std::list<HighwaySystem*> *hs_list, std::list<HighwaySystem*>::iterator *it, std::mutex *mtx)
{	//printf("Starting CompStatsRThread %02i\n", id); fflush(stdout);
	while (*it != hs_list->end())
	{	mtx->lock();
		if (*it == hs_list->end())
		{	mtx->unlock();
			return;
		}
		HighwaySystem *h(**it);
		//printf("CompStatsRThread %02i assigned %s\n", id, h->systemname.data()); fflush(stdout);
		(*it)++;
		//printf("CompStatsRThread %02i (*it)++ OK. Releasing lock.\n", id); fflush(stdout);
		mtx->unlock();
		std::cout << '.' << std::flush;
		for (Route &r : h->route_list)
		  for (HighwaySegment *s : r.segment_list)
		    s->compute_stats_r();
	}
}
