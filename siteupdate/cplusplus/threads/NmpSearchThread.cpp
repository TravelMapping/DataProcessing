void NmpSearchThread(unsigned int id, std::mutex* hs_mtx, WaypointQuadtree* all_waypoints)
{	//printf("Starting NmpSearchThread %02i\n", id); fflush(stdout);
	while (HighwaySystem::it != HighwaySystem::syslist.end())
	{	hs_mtx->lock();
		if (HighwaySystem::it == HighwaySystem::syslist.end())
		{	hs_mtx->unlock();
			return;
		}
		HighwaySystem* h(*HighwaySystem::it);
		//printf("NmpSearchThread %02i assigned %s\n", id, h->systemname.data()); fflush(stdout);
		HighwaySystem::it++;
		//printf("NmpSearchThread %02i HighwaySystem::it++\n", id); fflush(stdout);
		hs_mtx->unlock();
		for (Route *r : h->route_list)
		  for (Waypoint *w : r->point_list)
		    w->near_miss_points = all_waypoints->near_miss_waypoints(w, 0.0005);
	}
}
