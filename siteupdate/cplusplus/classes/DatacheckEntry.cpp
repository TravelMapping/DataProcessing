class DatacheckEntry
{   /* This class encapsulates a datacheck log entry

    route is a pointer to the route with a datacheck error

    labels is a list of labels that are related to the error (such
    as the endpoints of a too-long segment or the three points that
    form a sharp angle)

    code is the error code string, one of:
    BAD_ANGLE
    BUS_WITH_I
    DUPLICATE_COORDS
    DUPLICATE_LABEL
    HIDDEN_JUNCTION
    HIDDEN_TERMINUS
    INVALID_FINAL_CHAR
    INVALID_FIRST_CHAR
    LABEL_INVALID_CHAR
    LABEL_LOOKS_HIDDEN
    LABEL_PARENS
    LABEL_SELFREF
    LABEL_SLASHES
    LABEL_UNDERSCORES
    LACKS_GENERIC
    LONG_SEGMENT
    LONG_UNDERSCORE
    MALFORMED_LAT
    MALFORMED_LON
    MALFORMED_URL
    NONTERMINAL_UNDERSCORE
    OUT_OF_BOUNDS
    SHARP_ANGLE
    US_BANNER
    VISIBLE_DISTANCE
    VISIBLE_HIDDEN_COLOC

    info is additional information, at this time either a distance (in
    miles) for a long segment error, an angle (in degrees) for a sharp
    angle error, or a coordinate pair for duplicate coordinates, other
    route/label for point pair errors

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
