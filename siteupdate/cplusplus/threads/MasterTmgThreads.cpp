void MasterTmgSimpleThread(HighwayGraph *graph_data, GraphListEntry *msptr, std::string filename)
{	graph_data->write_master_tmg_simple(msptr, filename);
}

void MasterTmgCollapsedThread(HighwayGraph *graph_data, GraphListEntry *mcptr, std::string filename, std::list<TravelerList*> *traveler_lists)
{	graph_data->write_master_tmg_collapsed(mcptr, filename, 1, traveler_lists);
}

void MasterTmgTraveledThread(HighwayGraph *graph_data, GraphListEntry *mtptr, std::string filename, std::list<TravelerList*> *traveler_lists)
{	graph_data->write_master_tmg_traveled(mtptr, filename, 1, traveler_lists);
}
