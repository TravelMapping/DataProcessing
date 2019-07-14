void UserLogThread
(	unsigned int id, std::list<TravelerList*> *tl_list, std::list<TravelerList*>::iterator *it,
	std::mutex *mtx, ClinchedDBValues *clin_db_val, const double total_active_only_miles,
	const double total_active_preview_miles, std::list<HighwaySystem*> *highway_systems, std::string path
)
{	//printf("Starting UserLogThread %02i\n", id); fflush(stdout);
	while (*it != tl_list->end())
	{	mtx->lock();
		if (*it == tl_list->end())
		{	mtx->unlock();
			return;
		}
		TravelerList *t(**it);
		//printf("UserLogThread %02i assigned %s\n", id, t->traveler_name.data()); fflush(stdout);
		(*it)++;
		//printf("UserLogThread %02i (*it)++\n", id); fflush(stdout);
		mtx->unlock();
		t->userlog(clin_db_val, total_active_only_miles, total_active_preview_miles, highway_systems, path);
	}
}
