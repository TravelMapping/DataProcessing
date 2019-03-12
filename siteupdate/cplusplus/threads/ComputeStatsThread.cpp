void ComputeStatsThread(unsigned int id, std::list<HighwaySystem*> *hs_list, std::mutex *mtx)
{	//std::cout << "Starting ComputeStatsThread " << id << std::endl;
	while (hs_list->size())
	{	mtx->lock();
		if (!hs_list->size())
		{	mtx->unlock();
			return;
		}
		//std::cout << "Thread " << id << " with hs_list->size()=" << hs_list->size() << std::endl;
		HighwaySystem *h(hs_list->front());
		//std::cout << "Thread " << id << " assigned " << h->systemname << std::endl;
		hs_list->pop_front();
		//std::cout << "Thread " << id << " hs_list->pop_front() successful." << std::endl;
		mtx->unlock();
		std::cout << '.' << std::flush;
		for (Route &r : h->route_list)
		  for (HighwaySegment *s : r.segment_list)
		    s->compute_stats();
	}
}
