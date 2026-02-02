void MasterTmgThread(HighwayGraph* graph_data, std::mutex* l, std::mutex* t, WaypointQuadtree *qt, ElapsedTime *et)
{	HGVertex::vnums = new int[graph_data->vertices.size()*3];
	graph_data->write_master_graphs_tmg();
	delete[] HGVertex::vnums;
	SubgraphThread(0, l, t, graph_data, qt, et);
}
