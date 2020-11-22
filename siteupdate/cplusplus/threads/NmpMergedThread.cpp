void NmpMergedThread(unsigned int id, std::list<HighwaySystem*> *hs_list, std::list<HighwaySystem*>::iterator *it, std::mutex *mtx, std::string *nmpmergepath)
{	//printf("Starting NMPMergedThread %02i\n", id); fflush(stdout);
	while (*it != hs_list->end())
	{	mtx->lock();
		if (*it == hs_list->end())
		{	mtx->unlock();
			return;
		}
		HighwaySystem *h(**it);
		//printf("NmpMergedThread %02i assigned %s\n", id, h->systemname.data()); fflush(stdout);
		(*it)++;
		//printf("NmpMergedThread %02i (*it)++\n", id); fflush(stdout);
		mtx->unlock();
		std::cout << h->systemname << '.' << std::flush;
		for (Route &r : h->route_list)
			r.write_nmp_merged(*nmpmergepath + "/" + r.rg_str);
	}
}
