void ReadListThread(unsigned int id, std::mutex* tl_mtx, ErrorList* el)
{	//printf("Starting ReadListThread %02i\n", id); fflush(stdout);
	while (TravelerList::id_it != TravelerList::ids.end())
	{	tl_mtx->lock();
		if (TravelerList::id_it == TravelerList::ids.end())
		{	tl_mtx->unlock();
			return;
		}
		std::string& tl(*TravelerList::id_it);
		//printf("ReadListThread %02i assigned %s\n", id, tl.data()); fflush(stdout);
		TravelerList::id_it++;
		//printf("ReadListThread %02i (*it)++\n", id); fflush(stdout);
		tl_mtx->unlock();
		TravelerList *t = new TravelerList(tl, el);
				  // deleted on termination of program
		TravelerList::mtx.lock();
		TravelerList::allusers.push_back(t);
		TravelerList::mtx.unlock();
	}
}
