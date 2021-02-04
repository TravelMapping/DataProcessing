void SubgraphThread
(	unsigned int id, HighwayGraph *graph_data, std::vector<GraphListEntry> *graph_vector,
	size_t *index, std::mutex *l, std::mutex *t, std::string path, WaypointQuadtree *qt, ElapsedTime *et
)
{	//std::cout << "Starting SubgraphThread " << id << std::endl;
	while (*index < graph_vector->size())
	{	l->lock();
		if (*index >= graph_vector->size())
		{	l->unlock();
			return;
		}
		//std::cout << "Thread " << id << " with graph_vector.size()=" << graph_vector->size() << " & index=" << *index << std::endl;
		//std::cout << "Thread " << id << " assigned " << graph_vector->at(*index).tag() << std::endl;
		size_t i = *index;
		*index += 3;
		l->unlock();
		graph_data->write_subgraphs_tmg(*graph_vector, path, i, id, qt, et, t);
	}
}
