      #ifdef threading_enabled
	TravelerList::tl_it = TravelerList::allusers.begin();
	THREADLOOP thr[t] = thread(UserLogThread, t, &list_mtx, active_only_miles, active_preview_miles);
	THREADLOOP thr[t].join();
      #else
	for (TravelerList& t : TravelerList::allusers)
		t.userlog(active_only_miles, active_preview_miles);
      #endif
	cout << "!" << endl;
