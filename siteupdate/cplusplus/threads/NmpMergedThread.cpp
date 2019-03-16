void NmpMergedThread(std::list<HighwaySystem*> *hs_list, std::mutex *mtx, std::string *nmpmergepath)
{	//std::cout << "Starting NMPMergedThread " << id << std::endl;
	while (hs_list->size())
	{	mtx->lock();
		if (!hs_list->size())
		{	mtx->unlock();
			return;
		}
		//std::cout << "Thread " << id << " with hs_list.size()=" << hs_list.size() << std::endl;
		HighwaySystem *h(hs_list->front());
		//std::cout << "Thread " << id << " assigned " << h->systemname << std::endl;
		hs_list->pop_front();
		//std::cout << "Thread " << id << " hs_list->pop_front() successful." << std::endl;
		mtx->unlock();
		for (Route &r : h->route_list)
			r.write_nmp_merged(*nmpmergepath + "/" + r.region->code);
	}
}
