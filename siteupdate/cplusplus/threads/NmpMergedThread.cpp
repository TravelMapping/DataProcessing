void NmpMergedThread(unsigned int id, std::mutex* mtx)
{	//printf("Starting NMPMergedThread %02i\n", id); fflush(stdout);
	while (HighwaySystem::it != HighwaySystem::syslist.end())
	{	mtx->lock();
		if (HighwaySystem::it == HighwaySystem::syslist.end())
		{	mtx->unlock();
			return;
		}
		HighwaySystem* h(*HighwaySystem::it);
		//printf("NmpMergedThread %02i assigned %s\n", id, h->systemname.data()); fflush(stdout);
		HighwaySystem::it++;
		//printf("NmpMergedThread %02i HighwaySystem::it++\n", id); fflush(stdout);
		mtx->unlock();
		std::cout << h->systemname << '.' << std::flush;
		for (Route *r : h->route_list)
			r->write_nmp_merged();
	}
}
