      #ifdef threading_enabled
	HighwaySystem::it = HighwaySystem::syslist.begin();
	THREADLOOP thr[t] = thread(NmpMergedThread, t, &list_mtx);
	THREADLOOP thr[t].join();
      #else
	for (HighwaySystem& h : HighwaySystem::syslist)
	{	std::cout << h.systemname << std::flush;
		for (Route& r : h.routes)
		  r.write_nmp_merged();
		std::cout << '.' << std::flush;
	}
      #endif
	cout << endl;
