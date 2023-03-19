void ReadWptThread
(	unsigned int id, std::mutex* hs_mtx,
	ErrorList* el,WaypointQuadtree* all_waypoints
)
{	//printf("Starting ReadWptThread %02i\n", id); fflush(stdout);
	while (HighwaySystem::it != HighwaySystem::syslist.end())
	{	hs_mtx->lock();
		if (HighwaySystem::it == HighwaySystem::syslist.end())
		{	hs_mtx->unlock();
			return;
		}
		HighwaySystem* h(*HighwaySystem::it);
		//printf("ReadWptThread %02i assigned %s\n", id, h->systemname.data()); fflush(stdout);
		HighwaySystem::it++;
		//printf("ReadWptThread %02i HighwaySystem::it++\n", id); fflush(stdout);
		hs_mtx->unlock();
		std::cout << h->systemname << ' ' << std::flush;
		bool usa_flag = h->country->first == "USA";
		Region* prev_region = nullptr;
		for (Route *r : h->route_list)
		{	// create key/value pairs in h->mileage_by_region, to be computed in a threadsafe manner later
			if (r->region != prev_region) // avoid unnecessary hashing when we know a region's already been inserted
			{	h->mileage_by_region[r->region];
				prev_region = r->region;
			}
			r->read_wpt(all_waypoints, el, usa_flag);
		}
		//std::cout << "!" << std::endl;
	}
}
