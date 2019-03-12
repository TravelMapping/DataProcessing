void ReadWptThread
(	unsigned int id, std::list<HighwaySystem*> *hs_list, std::mutex *hs_mtx,
	std::string path, ErrorList *el, std::unordered_set<std::string> *all_wpt_files,
	WaypointQuadtree *all_waypoints, std::mutex *strtok_mtx, DatacheckEntryList *datacheckerrors
)
{	//std::cout << "Starting ReadWptThread " << id << std::endl;
	while (hs_list->size())
	{	hs_mtx->lock();
		if (!hs_list->size())
		{	hs_mtx->unlock();
			return;
		}
		//std::cout << "Thread " << id << " with hs_list->size()=" << hs_list->size() << std::endl;
		HighwaySystem *h(hs_list->front());
		//std::cout << "Thread " << id << " assigned " << h->systemname << std::endl;
		hs_list->pop_front();
		//std::cout << "Thread " << id << " hs_list->pop_front() successful." << std::endl;
		hs_mtx->unlock();
		std::cout << h->systemname << std::flush;
		for (Route &r : h->route_list)
			r.read_wpt(all_waypoints, el, path, strtok_mtx, datacheckerrors, all_wpt_files);
		std::cout << "!" << std::endl;
	}
}
