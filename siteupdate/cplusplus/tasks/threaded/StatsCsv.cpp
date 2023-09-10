      #ifdef threading_enabled
	HighwaySystem::it = HighwaySystem::syslist.begin();
	switch(Args::mtcsvfiles ? Args::numthreads : 1)
	{   case 1:
      #endif
		cout << et.et() << "Writing allbyregionactiveonly.csv." << endl;
		allbyregionactiveonly(0, active_only_miles);
		cout << et.et() << "Writing allbyregionactivepreview.csv." << endl;
		allbyregionactivepreview(0, active_preview_miles);
		cout << et.et() << "Writing per-system stats csv files." << endl;
		for (HighwaySystem& h : HighwaySystem::syslist) h.stats_csv();
      #ifdef threading_enabled
		break;
	   case 2:
		thr[0] = thread(allbyregionactiveonly,    &list_mtx, active_only_miles);
		thr[1] = thread(allbyregionactivepreview, &list_mtx, active_preview_miles);
		thr[0].join();
		thr[1].join();
		break;
	   default:
		thr[0] = thread(allbyregionactiveonly,    nullptr, active_only_miles);
		thr[1] = thread(allbyregionactivepreview, nullptr, active_preview_miles);
		thr[2] = thread(StatsCsvThread, 2, &list_mtx);
		thr[0].join();
		thr[1].join();
		thr[2].join();
	}
      #endif
