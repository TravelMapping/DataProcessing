void NmpSearchThread(unsigned int id, std::mutex* mtx, WaypointQuadtree* all_waypoints)
{	//printf("Starting NmpSearchThread %02i\n", id); fflush(stdout);
	while (HighwaySystem::it != HighwaySystem::syslist.end())
	{	mtx->lock();
		if (HighwaySystem::it == HighwaySystem::syslist.end())
			return mtx->unlock();
		HighwaySystem* h = HighwaySystem::it++;
		mtx->unlock();

		for (Route& r : h->routes)
		  for (Waypoint& w : r.points)
		    all_waypoints->near_miss_waypoints(&w, Args::nmpthreshold);
	}
}
