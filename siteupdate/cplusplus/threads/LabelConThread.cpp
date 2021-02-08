void LabelConThread
(	unsigned int id, std::list<HighwaySystem*>* hs_list,
	std::list<HighwaySystem*>::iterator* it, std::mutex* hs_mtx,  ErrorList* el
)
{	//printf("Starting LabelConThread %02i\n", id); fflush(stdout);
	while (*it != hs_list->end())
	{	hs_mtx->lock();
		if (*it == hs_list->end())
		{	hs_mtx->unlock();
			return;
		}
		HighwaySystem *h(**it);
		//printf("LabelConThread %02i assigned %s\n", id, h->systemname.data()); fflush(stdout);
		(*it)++;
		//printf("LabelConThread %02i (*it)++\n", id); fflush(stdout);
		hs_mtx->unlock();
		for (Route* r : h->route_list) r->label_and_connect(*el);
	}
}
