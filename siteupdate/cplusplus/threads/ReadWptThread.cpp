void ReadWptThread
(	unsigned int id, std::list<HighwaySystem*> *hs_list, std::list<HighwaySystem*>::iterator *it,
	std::mutex *hs_mtx, std::string path, ErrorList *el, std::unordered_set<std::string> *all_wpt_files,
	WaypointQuadtree *all_waypoints, std::mutex *strtok_mtx, DatacheckEntryList *datacheckerrors
)
{	//printf("Starting ReadWptThread %02i\n", id); fflush(stdout);
	while (*it != hs_list->end())
	{	hs_mtx->lock();
		if (*it == hs_list->end())
		{	hs_mtx->unlock();
			return;
		}
		HighwaySystem *h(**it);
		//printf("ReadWptThread %02i assigned %s\n", id, h->systemname.data()); fflush(stdout);
		(*it)++;
		//printf("ReadWptThread %02i (*it)++\n", id); fflush(stdout);
		hs_mtx->unlock();
		std::cout << h->systemname << std::flush;
		for (Route &r : h->route_list)
			r.read_wpt(all_waypoints, el, path, strtok_mtx, datacheckerrors, all_wpt_files);
		std::cout << "!" << std::endl;
	}
}
