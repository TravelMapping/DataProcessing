class HighwaySegment
{   /* This class represents one highway segment: the connection between two
    Waypoints connected by one or more routes */

	public:
	Waypoint *waypoint1;
	Waypoint *waypoint2;
	Route *route;
	std::list<HighwaySegment*> *concurrent;
	std::unordered_set<TravelerList*> clinched_by;
	std::mutex clin_mtx;

	HighwaySegment(Waypoint *, Waypoint *, Route *);

	std::string str();
	bool add_clinched_by(TravelerList *);
	std::string csv_line(unsigned int);
	double length();
	std::string segment_name();
	unsigned int index();
	void compute_stats();
	std::string clinchedby_code(std::list<TravelerList*> *);
};
