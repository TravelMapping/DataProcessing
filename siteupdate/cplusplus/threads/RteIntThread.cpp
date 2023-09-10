void RteIntThread(unsigned int id, std::mutex* mtx,  ErrorList* el)
{	//printf("Starting LabelConThread %02i\n", id); fflush(stdout);
	while (HighwaySystem::it != HighwaySystem::syslist.end())
	{	mtx->lock();
		if (HighwaySystem::it == HighwaySystem::syslist.end())
			return mtx->unlock();
		HighwaySystem* h = HighwaySystem::it++;
		mtx->unlock();

		h->route_integrity(*el);
	}
}
