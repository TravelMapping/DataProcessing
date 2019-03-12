void UserLogThread(unsigned int id, std::list<TravelerList*> *tl_list, std::mutex *mtx, ClinchedDBValues *clin_db_val, const double total_active_only_miles, const double total_active_preview_miles, std::list<HighwaySystem*> *highway_systems, std::string path)
{	//std::cout << "Starting UserLogThread " << id << std::endl;
	while (tl_list->size())
	{	mtx->lock();
		if (!tl_list->size())
		{	mtx->unlock();
			return;
		}
		//std::cout << "Thread " << id << " with tl_list->size()=" << tl_list->size() << std::endl;
		TravelerList *t(tl_list->front());
		//std::cout << "Thread " << id << " assigned " << t->traveler_name << std::endl;
		tl_list->pop_front();
		//std::cout << "Thread " << id << " tl_list->pop_front() successful." << std::endl;
		mtx->unlock();
		t->userlog(clin_db_val, total_active_only_miles, total_active_preview_miles, highway_systems, path);
	}
}
