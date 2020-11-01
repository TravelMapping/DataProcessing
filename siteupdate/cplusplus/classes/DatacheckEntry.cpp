class DatacheckEntry
{   /* This class encapsulates a datacheck log entry

    route is a pointer to the route with a datacheck error

    labels is a list of labels that are related to the error (such
    as the endpoints of a too-long segment or the three points that
    form a sharp angle)

    code is the error code | info is additional
    string, one of:        | information, if used:
    -----------------------+--------------------------------------------
    BAD_ANGLE              |
    BUS_WITH_I             |
    DISCONNECTED_ROUTE     | adjacent root's expected connection point
    DUPLICATE_COORDS       | coordinate pair
    DUPLICATE_LABEL        |
    HIDDEN_JUNCTION        | number of incident edges in TM master graph
    HIDDEN_TERMINUS        |
    INTERSTATE_NO_HYPHEN   |
    INVALID_FINAL_CHAR     | final character in label
    INVALID_FIRST_CHAR     | first character in label other than *
    LABEL_INVALID_CHAR     |
    LABEL_LOOKS_HIDDEN     |
    LABEL_PARENS           |
    LABEL_SELFREF          |
    LABEL_SLASHES          |
    LABEL_TOO_LONG         |
    LABEL_UNDERSCORES      |
    LACKS_GENERIC          |
    LONG_SEGMENT           | distance in miles
    LONG_UNDERSCORE        |
    MALFORMED_LAT          | malformed "lat=" parameter from OSM url
    MALFORMED_LON          | malformed "lon=" parameter from OSM url
    MALFORMED_URL          | always "MISSING_ARG(S)"
    NONTERMINAL_UNDERSCORE |
    OUT_OF_BOUNDS          | coordinate pair
    SHARP_ANGLE            | angle in degrees
    US_BANNER              |
    VISIBLE_DISTANCE       | distance in miles
    VISIBLE_HIDDEN_COLOC   | hidden point at same coordinates

    fp is a boolean indicating whether this has been reported as a
    false positive (would be set to true later)

    */
	public:
	Route *route;
	std::string label1;
	std::string label2;
	std::string label3;
	std::string code;
	std::string info;
	bool fp;

	DatacheckEntry(Route *rte, std::string l1, std::string l2, std::string l3, std::string c, std::string i)
	{	route = rte;
		label1 = l1;
		label2 = l2;
		label3 = l3;
		code = c;
		info = i;
		fp = 0;
	}

	bool match_except_info(std::array<std::string, 6> &fpentry)
	{	// Check if the fpentry from the csv file matches in all fields
		// except the info field
		if (fpentry[0] != route->root)	return 0;
		if (fpentry[1] != label1)	return 0;
		if (fpentry[2] != label2)	return 0;
		if (fpentry[3] != label3)	return 0;
		if (fpentry[4] != code)		return 0;
		return 1;
	}

	// Original "Python list" format unused. Using "CSV style" format instead.
	std::string str()
	{	return route->root + ";" + label1 + ";" + label2 + ";" + label3 + ";" + code + ";" + info;
	}

	bool operator < (DatacheckEntry &other)
	{	return str() < other.str();
	}
};
