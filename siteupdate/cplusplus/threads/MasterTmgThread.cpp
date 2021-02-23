void MasterTmgThread(HighwayGraph* graph_data, std::mutex* l, std::mutex* t, WaypointQuadtree *qt, ElapsedTime *et)
{	graph_data->write_master_graphs_tmg();
	SubgraphThread(0, l, t, graph_data, qt, et);
}
