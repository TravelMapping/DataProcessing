	TravelerList::id_it = TravelerList::ids.begin();
      #ifdef threading_enabled
	THREADLOOP thr[t] = thread(ReadListThread, t, &list_mtx, &el);
	THREADLOOP thr[t].join();
      #else
	while (TravelerList::tl_it < TravelerList::allusers.end())
		new(TravelerList::tl_it++) TravelerList(*TravelerList::id_it++, &el);
		// placement new
      #endif
	if (TravelerList::file_not_found)
	{	cout << "\nCheck for typos in your -U or --userlist arguments, and make sure " << Args::userlistext << " files for all specified users exist.\nAborting." << endl;
		failure_cleanup(all_waypoints, colocate_counts, updates, systemupdates);
		return 1;
	}
	TravelerList::ids.clear();
