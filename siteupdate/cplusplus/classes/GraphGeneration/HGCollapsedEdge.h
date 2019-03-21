class HGEdge
{   /* This class encapsulates information needed for a highway graph
    edge that can incorporate intermediate points.
    */
	public:
	char written;
	std::string segment_name;
	HGVertex *vertex1, *vertex2;
	std::list<HGVertex*> intermediate_points; // if more than 1, will go from vertex1 to vertex2
	// canonical segment used to reference region and list of travelers
	// assumption: each edge/segment lives within a unique region
	// and a 'multi-edge' would not be able to span regions as there
	// would be a required visible waypoint at the border
	HighwaySegment *segment;
	std::list<std::pair<std::string, HighwaySystem*>> route_names_and_systems;

	HGEdge(HighwaySegment *, HighwayGraph *);
	HGEdge(HGVertex *);
	~HGEdge();

	void remove(std::list<HGEdge*> &);
	std::string label(std::list<HighwaySystem*> *);
	std::string collapsed_tmg_line(std::list<HighwaySystem*> *, unsigned int);
	std::string traveled_tmg_line(std::list<HighwaySystem*> *, unsigned int, std::list<TravelerList*> *);
	std::string debug_tmg_line(std::list<HighwaySystem*> *, unsigned int);
	std::string str();
	std::string intermediate_point_string();
	bool s_written();
	bool c_written();
	bool t_written();
};
