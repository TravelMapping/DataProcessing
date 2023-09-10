      #ifdef threading_enabled
	Region::it = Region::allregions.begin();
	THREADLOOP thr[t] = thread(CompStatsThread, t, &list_mtx);
	THREADLOOP thr[t].join();
      #else
	for (Region& rg : Region::allregions) rg.compute_stats();
      #endif
	cout << '!' << endl;
