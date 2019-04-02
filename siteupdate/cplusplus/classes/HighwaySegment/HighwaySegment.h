class HighwaySegment
{   /* This class represents one highway segment: the connection between two
    Waypoints connected by one or more routes */

	public:
	Waypoint *waypoint1;
	Waypoint *waypoint2;
	Route *route;
	std::list<HighwaySegment*> *concurrent;
	std::forward_list<TravelerList*> clinched_by;	//FIXME try unordered_set?
	std::mutex clin_mtx;

	HighwaySegment(Waypoint *, Waypoint *, Route *);

	std::string str();
	bool add_clinched_by(TravelerList *);		//FIXME rework to use unordered_set? Simpler coding, probably faster
	std::string csv_line(unsigned int);
	double length();
	std::string segment_name();
	unsigned int index();
	//std::string concurrent_travelers_sanity_check();
	void compute_stats();
};
