class HGEdge
{   /* This class encapsulates information needed for a highway graph
    edge that can incorporate intermediate points.
    */
	public:
	bool s_written, c_written, t_written; // simple, collapsed, traveled
	std::string segment_name;
	HGVertex *vertex1, *vertex2;
	std::list<HGVertex*> intermediate_points; // if more than 1, will go from vertex1 to vertex2
	HighwaySegment *segment;
	std::list<std::pair<std::string, HighwaySystem*>> route_names_and_systems;
	unsigned char format;

	// constants for more human-readable format masks
	static const unsigned char simple = 1;
	static const unsigned char collapsed = 2;
	static const unsigned char traveled = 4;

	HGEdge(HighwaySegment *, HighwayGraph *);
	HGEdge(HGVertex *, unsigned char);
	~HGEdge();

	void detach(unsigned char);
	std::string label(std::list<HighwaySystem*> *);
	std::string collapsed_tmg_line(std::list<HighwaySystem*> *, unsigned int);
	std::string traveled_tmg_line(std::list<HighwaySystem*> *, std::list<TravelerList*> *, unsigned int);
	std::string debug_tmg_line(std::list<HighwaySystem*> *, unsigned int);
	std::string str();
	std::string intermediate_point_string();
};
