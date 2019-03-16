class HighwayGraphCollapsedEdgeInfo
{   /* This class encapsulates information needed for a highway graph
    edge that can incorporate intermediate points.
    */
	public:
	bool written;
	std::string segment_name;
	HighwayGraphVertexInfo *vertex1, *vertex2;
	std::list<HighwayGraphVertexInfo*> intermediate_points; // if more than 1, will go from vertex1 to vertex2
	// canonical segment used to reference region and list of travelers
	// assumption: each edge/segment lives within a unique region
	// and a 'multi-edge' would not be able to span regions as there
	// would be a required visible waypoint at the border
	HighwaySegment *segment;
	std::list<std::pair<std::string, HighwaySystem*>> route_names_and_systems;

	HighwayGraphCollapsedEdgeInfo(HighwayGraphEdgeInfo *);
	HighwayGraphCollapsedEdgeInfo(HighwayGraphVertexInfo *);
	~HighwayGraphCollapsedEdgeInfo();

	std::string label(std::list<HighwaySystem*> *);
	std::string collapsed_tmg_line(std::list<HighwaySystem*> *, unsigned int);
	std::string collapsed_tmg2_line(std::list<HighwaySystem*> *, unsigned int, std::list<TravelerList*> *);
	std::string debug_tmg_line(std::list<HighwaySystem*> *, unsigned int);
	std::string str();
	std::string intermediate_point_string();
};
