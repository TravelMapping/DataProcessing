void MasterTmgThread(	HighwayGraph *graph_data, std::vector<GraphListEntry> *graph_vector,
			std::string path, std::list<TravelerList*> *traveler_lists,
			size_t *index, std::mutex *mtx, WaypointQuadtree *qt, ElapsedTime *et)
{	graph_data->write_master_graphs_tmg(*graph_vector, path, *traveler_lists);
	SubgraphThread(0, graph_data, graph_vector, index, mtx, path, qt, et);
}
