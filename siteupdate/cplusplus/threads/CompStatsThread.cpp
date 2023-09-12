void CompStatsThread(unsigned int id, std::mutex* mtx)
{	//printf("Starting CompStatsThread %02i\n", id); fflush(stdout);
	while (Region::it != Region::allregions.end())
	{	mtx->lock();
		if (Region::it == Region::allregions.end())
			return mtx->unlock();
		Region* rg = Region::it++;
		mtx->unlock();

		rg->compute_stats();
	}
}
