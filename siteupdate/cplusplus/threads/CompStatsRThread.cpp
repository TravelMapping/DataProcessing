void CompStatsRThread(unsigned int id, std::mutex* mtx)
{	//printf("Starting CompStatsRThread %02i\n", id); fflush(stdout);
	while (HighwaySystem::it != HighwaySystem::syslist.end())
	{	mtx->lock();
		if (HighwaySystem::it == HighwaySystem::syslist.end())
		{	mtx->unlock();
			return;
		}
		HighwaySystem* h(*HighwaySystem::it);
		//printf("CompStatsRThread %02i assigned %s\n", id, h->systemname.data()); fflush(stdout);
		HighwaySystem::it++;
		//printf("CompStatsRThread %02i HighwaySystem::it++ OK. Releasing lock.\n", id); fflush(stdout);
		mtx->unlock();
		std::cout << '.' << std::flush;
		for (Route *r : h->route_list)
		  r->compute_stats_r();
	}
}
