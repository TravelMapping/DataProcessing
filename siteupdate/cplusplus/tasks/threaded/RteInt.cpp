      #ifdef threading_enabled
	HighwaySystem::it = HighwaySystem::syslist.begin();
	THREADLOOP thr[t] = thread(RteIntThread, t, &list_mtx, &el);
	THREADLOOP thr[t].join();
      #else
	for (HighwaySystem& h : HighwaySystem::syslist)
	  h.route_integrity(el);
      #endif
