void ReadListThread(std::list<std::string> *traveler_ids, std::list<TravelerList*> *traveler_lists, std::mutex *tl_mtx, std::mutex *strtok_mtx, Arguments *args, std::unordered_map<std::string, Route*> *route_hash)
{	//std::cout << "Starting ReadListThread " << id << std::endl;
	while (traveler_ids->size())
	{	tl_mtx->lock();
		if (!traveler_ids->size())
		{	tl_mtx->unlock();
			return;
		}
		//std::cout << "Thread " << id << " with traveler_ids.size()=" << traveler_ids.size() << std::endl;
		std::string tl(traveler_ids->front());
		//std::cout << "Thread " << id << " assigned " << tl << std::endl;
		traveler_ids->pop_front();
		//std::cout << "Thread " << id << " traveler_ids->pop_front() successful." << std::endl;
		std::cout << ' ' << tl << std::flush;
		tl_mtx->unlock();
		TravelerList *t = new TravelerList(tl, route_hash, args, strtok_mtx);
				  // deleted on termination of program
		TravelerList::alltrav_mtx.lock();
		traveler_lists->push_back(t);
		TravelerList::alltrav_mtx.unlock();
	}
}
