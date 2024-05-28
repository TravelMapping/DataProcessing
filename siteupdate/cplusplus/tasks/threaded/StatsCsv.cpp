      #ifdef threading_enabled
	HighwaySystem::it = HighwaySystem::syslist.begin();
	if (Args::numthreads == 1 || Args::stcsvfiles)
      #endif
	     {	cout << et.et() << "Writing allbyregionactiveonly.csv." << endl;
		allbyregionactiveonly(0, active_only_miles);
		cout << et.et() << "Writing allbyregionactivepreview.csv." << endl;
		allbyregionactivepreview(0, active_preview_miles);
		cout << et.et() << "Writing per-system stats csv files." << endl;
		for (HighwaySystem& h : HighwaySystem::syslist) h.stats_csv();
	     }
      #ifdef threading_enabled
	else {	thr[0] = thread(allbyregionactiveonly,    &list_mtx, active_only_miles);
		thr[1] = thread(allbyregionactivepreview, &list_mtx, active_preview_miles);
		// start at t=2, because allbyregionactive* will spawn another StatsCsvThread when finished
		for (unsigned int t = 2; t < thr.size(); t++) thr[t] = thread(StatsCsvThread, t, &list_mtx);
		THREADLOOP thr[t].join();
	     }
      #endif

