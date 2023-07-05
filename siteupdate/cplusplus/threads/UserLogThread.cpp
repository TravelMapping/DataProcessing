void UserLogThread(unsigned int id, std::mutex* mtx, const double ao_mi, const double ap_mi)
{	//printf("Starting UserLogThread %02i\n", id); fflush(stdout);
	while (TravelerList::tl_it != TravelerList::allusers.end())
	{	mtx->lock();
		if (TravelerList::tl_it == TravelerList::allusers.end())
			return mtx->unlock();
		TravelerList* t = TravelerList::tl_it++;
		mtx->unlock();

		t->userlog(ao_mi, ap_mi);
	}
}
