void StatsCsvThread(unsigned int id, std::mutex* mtx)
{	//printf("Starting StatsCsvThread %02i\n", id); fflush(stdout);
	while (HighwaySystem::it != HighwaySystem::syslist.end())
	{	mtx->lock();
		if (HighwaySystem::it == HighwaySystem::syslist.end())
			return mtx->unlock();
		HighwaySystem *h = HighwaySystem::it++;
		mtx->unlock();

		h->stats_csv();
	}
}
