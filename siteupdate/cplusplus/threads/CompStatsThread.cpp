void CompStatsThread(unsigned int id, std::mutex* list_mtx)
{	//printf("Starting CompStatsThread %02i\n", id); fflush(stdout);
	while (Region::rg_it != Region::allregions.end())
	{	list_mtx->lock();
		if (Region::rg_it == Region::allregions.end())
		{	list_mtx->unlock();
			return;
		}
		Region* rg(*Region::rg_it);
		//printf("CompStatsThread %02i assigned %s\n", id, rg->code.data()); fflush(stdout);
		Region::rg_it++;
		//printf("CompStatsThread %02i Region::rg_it++\n", id); fflush(stdout);
		list_mtx->unlock();
		rg->compute_stats();
	}
}
