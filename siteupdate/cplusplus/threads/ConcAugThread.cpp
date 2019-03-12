void ConcAugThread(unsigned int id, std::list<TravelerList*> *travlists, std::mutex *tl_mtx, std::mutex *log_mtx, std::ofstream *concurrencyfile)
{	//std::cout << "Starting NMPMergedThread " << id << std::endl;
	while (travlists->size())
	{	tl_mtx->lock();
		if (!travlists->size())
		{	tl_mtx->unlock();
			return;
		}
		//std::cout << "Thread " << id << " with travlists.size()=" << travlists->size() << std::endl;
		TravelerList *t(travlists->front());
		//std::cout << "Thread " << id << " assigned " << t->traveler_name << std::endl;
		travlists->pop_front();
		//std::cout << "Thread " << id << " travlists->pop_front() successful." << std::endl;
		tl_mtx->unlock();
		std::cout << '.' << std::flush;
		for (HighwaySegment *s : t->clinched_segments)
		  if (s->concurrent)
		    for (HighwaySegment *hs : *(s->concurrent))
		      if (hs->route->system->active_or_preview() && hs->add_clinched_by(t))
		      {	log_mtx->lock();
			*concurrencyfile << "Concurrency augment for traveler " << t->traveler_name << ": [" << hs->str() << "] based on [" << s->str() << "]\n";
			log_mtx->unlock();
		      }
	}
}
