void UserLogThread
(	unsigned int id, std::mutex* mtx, ClinchedDBValues* clin_db_val,
	const double total_active_only_miles,
	const double total_active_preview_miles
)
{	//printf("Starting UserLogThread %02i\n", id); fflush(stdout);
	while (TravelerList::tl_it != TravelerList::allusers.end())
	{	mtx->lock();
		if (TravelerList::tl_it == TravelerList::allusers.end())
		{	mtx->unlock();
			return;
		}
		TravelerList* t(*TravelerList::tl_it);
		//printf("UserLogThread %02i assigned %s\n", id, t->traveler_name.data()); fflush(stdout);
		TravelerList::tl_it++;
		//printf("UserLogThread %02i TravelerList::tl_it++\n", id); fflush(stdout);
		mtx->unlock();
		t->userlog(clin_db_val, total_active_only_miles, total_active_preview_miles);
	}
}
