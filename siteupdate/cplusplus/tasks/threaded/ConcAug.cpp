      #ifdef threading_enabled
	auto augment_lists = new vector<string>[Args::numthreads];
				      // deleted once written to concurrencies.log
	TravelerList::tl_it = TravelerList::allusers.begin();
	THREADLOOP thr[t] = thread(ConcAugThread, t, &list_mtx, augment_lists+t);
	THREADLOOP thr[t].join();
	cout << "!\n" << et.et() << "Writing to concurrencies.log." << endl;
	THREADLOOP for (std::string& entry : augment_lists[t]) concurrencyfile << entry << '\n';
	delete[] augment_lists;
      #else
	for (TravelerList *t = TravelerList::allusers.data, *end = TravelerList::allusers.end(); t != end; t++)
	{	cout << '.' << flush;
		size_t index = t-TravelerList::allusers.data;
		for (HighwaySegment *s : t->clinched_segments)
		  if (s->concurrent)
		    for (HighwaySegment *hs : *(s->concurrent))
		      if (hs != s && hs->route->system->active_or_preview() && hs->clinched_by.add_index(index))
		       	concurrencyfile << "Concurrency augment for traveler " << t->traveler_name << ": [" << hs->str() << "] based on [" << s->str() << "]\n";
	}
	cout << '!' << endl;
      #endif
	concurrencyfile.close();
