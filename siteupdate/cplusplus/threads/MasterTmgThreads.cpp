void MasterTmgSimpleThread(HighwayGraph *graph_data, GraphListEntry *msptr, std::string filename)
{	graph_data->write_master_tmg_simple(msptr, filename);
}

void MasterTmgCollapsedThread(HighwayGraph *graph_data, GraphListEntry *mcptr, std::string filename)
{	graph_data->write_master_tmg_collapsed(mcptr, filename, 1);
}
