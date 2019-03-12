class ClinchedSegmentEntry
{   /* This class encapsulates one line of a traveler's list file

    raw_line is the actual line from the list file for error reporting

    route is a pointer to the route clinched

    start and end are indexes to the point_list vector in the Route object,
    in the same order as they appear in the route decription file
    */
	public:
	//std::string raw_line; // Implemented but unused in siteupdate.py. Commented out to conserve RAM.
	Route *route;
	unsigned int start, end;

	ClinchedSegmentEntry(/*std::string line,*/ Route *r, unsigned int start_i, unsigned int end_i)
	{	//raw_line = line;
		route = r;
		start = start_i;
		end = end_i;
	}
};
