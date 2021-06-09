void RteIntThread(unsigned int id, std::mutex* hs_mtx,  ErrorList* el)
{	//printf("Starting LabelConThread %02i\n", id); fflush(stdout);
	while (HighwaySystem::it != HighwaySystem::syslist.end())
	{	hs_mtx->lock();
		if (HighwaySystem::it == HighwaySystem::syslist.end())
		{	hs_mtx->unlock();
			return;
		}
		HighwaySystem* h(*HighwaySystem::it);
		//printf("LabelConThread %02i assigned %s\n", id, h->systemname.data()); fflush(stdout);
		HighwaySystem::it++;
		//printf("LabelConThread %02i HighwaySystem::it++\n", id); fflush(stdout);
		hs_mtx->unlock();
		h->route_integrity(*el);
	}
}
