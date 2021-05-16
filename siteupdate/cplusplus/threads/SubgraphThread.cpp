void SubgraphThread
(	unsigned int id, std::mutex* l, std::mutex* t,
	HighwayGraph* graph_data, WaypointQuadtree* qt, ElapsedTime* et
)
{	//std::cout << "Starting SubgraphThread " << id << std::endl;
	while (GraphListEntry::num < GraphListEntry::entries.size())
	{	l->lock();
		if (GraphListEntry::num >= GraphListEntry::entries.size())
		{	l->unlock();
			return;
		}
		//std::cout << "Thread " << id << " with graph_vector.size()=" << graph_vector->size() << " & index=" << GraphListEntry::num << std::endl;
		//std::cout << "Thread " << id << " assigned " << graph_vector->at(GraphListEntry::num).tag() << std::endl;
		size_t i = GraphListEntry::num;
		GraphListEntry::num += 3;
		l->unlock();
		graph_data->write_subgraphs_tmg(i, id, qt, et, t);
	}
}
