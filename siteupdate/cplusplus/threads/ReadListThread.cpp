void ReadListThread
(	unsigned int id, std::list<std::string> *traveler_ids, std::unordered_map<std::string, std::string**> *listupdates,
	std::list<std::string>::iterator *it, std::list<TravelerList*> *traveler_lists, std::mutex *tl_mtx, Arguments *args, ErrorList *el
)
{	//printf("Starting ReadListThread %02i\n", id); fflush(stdout);
	while (*it != traveler_ids->end())
	{	tl_mtx->lock();
		if (*it == traveler_ids->end())
		{	tl_mtx->unlock();
			return;
		}
		std::string tl(**it);
		//printf("ReadListThread %02i assigned %s\n", id, tl.data()); fflush(stdout);
		(*it)++;
		//printf("ReadListThread %02i (*it)++\n", id); fflush(stdout);
		std::cout << tl << ' ' << std::flush;
		tl_mtx->unlock();
		std::string** update;
		try {	update = listupdates->at(tl);
		    }
		catch (const std::out_of_range& oor)
		    {	update = 0;
		    }
		TravelerList *t = new TravelerList(tl, update, el, args);
				  // deleted on termination of program
		TravelerList::alltrav_mtx.lock();
		traveler_lists->push_back(t);
		TravelerList::alltrav_mtx.unlock();
	}
}
