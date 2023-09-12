      #ifdef threading_enabled
	HighwaySystem::it = HighwaySystem::syslist.begin();
	THREADLOOP thr[t] = thread(ReadWptThread, t, &list_mtx, &el, &all_waypoints);
	THREADLOOP thr[t].join();
      #else
	for (HighwaySystem& h : HighwaySystem::syslist)
	{	std::cout << h.systemname << ' ' << std::flush;
		bool usa_flag = h.country->first == "USA";
		for (Route& r : h.routes)
			r.read_wpt(&all_waypoints, &el, usa_flag);
		//std::cout << "!" << std::endl;
	}
      #endif
	std::cout << std::endl;
