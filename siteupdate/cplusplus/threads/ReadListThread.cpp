void ReadListThread(unsigned int num, std::mutex* mtx, ErrorList* el)
{	//printf("Starting ReadListThread %02i\n", num); fflush(stdout);
	while (TravelerList::id_it != TravelerList::ids.end())
	{	mtx->lock();
		if (TravelerList::id_it == TravelerList::ids.end())
			return mtx->unlock();
		std::string& id = *TravelerList::id_it++;
		TravelerList* tl = TravelerList::tl_it++;
		mtx->unlock();

		new(tl) TravelerList(id, el);
		// placement new
	}
}
