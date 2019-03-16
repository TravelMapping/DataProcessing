class HGEdge
{   /* This class encapsulates information needed for a 'standard'
    highway graph edge.
    */
	public:
	bool written;
	std::string segment_name;
	HGVertex *vertex1, *vertex2;
	HighwaySegment *segment;
	std::list<std::pair<std::string, HighwaySystem*>> route_names_and_systems;

	HGEdge(HighwaySegment *, HighwayGraph *, bool &);
	~HGEdge();

	std::string label(std::list<HighwaySystem*> *);
	std::string str();
};
