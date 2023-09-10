void NmpMergedThread(unsigned int id, std::mutex* mtx)
{	//printf("Starting NMPMergedThread %02i\n", id); fflush(stdout);
	while (HighwaySystem::it != HighwaySystem::syslist.end())
	{	mtx->lock();
		if (HighwaySystem::it == HighwaySystem::syslist.end())
			return mtx->unlock();
		HighwaySystem* h = HighwaySystem::it++;
		mtx->unlock();

		std::cout << h->systemname << '.' << std::flush;
		for (Route& r : h->routes)
			r.write_nmp_merged();
	}
}
