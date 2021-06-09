class ElapsedTime;
class ErrorList;
class Route;
#include <list>
#include <mutex>
#include <string>
#include <unordered_set>

class Datacheck
{   /* This class encapsulates a datacheck log entry

    route is a pointer to the route with a datacheck error

    label1, label2 & label3 are labels that are related to the error
    (such as the endpoints of a too-long segment or the three points
    that form a sharp angle)

    code is the error code | info is additional
    string, one of:        | information, if used:
    -----------------------+--------------------------------------------
    ABBREV_AS_CHOP_BANNER  | offending line # in chopped route CSV
    ABBREV_AS_CON_BANNER   | offending line # in connected route CSV
    BAD_ANGLE              |
    BUS_WITH_I             |
    CON_BANNER_MISMATCH    | Banner field in chopped & connected CSVs
    CON_ROUTE_MISMATCH     | Route field in chopped & connected CSVs
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
    US_LETTER              |
    VISIBLE_DISTANCE       | distance in miles
    VISIBLE_HIDDEN_COLOC   | hidden point at same coordinates

    fp is a boolean indicating whether this has been reported as a
    false positive (would be set to true later)

    */
	static std::mutex mtx;
	static std::list<std::string*> fps;
	static std::unordered_set<std::string> always_error;
	public:
	Route *route;
	std::string label1;
	std::string label2;
	std::string label3;
	std::string code;
	std::string info;
	bool fp;

	static std::list<Datacheck> errors;
	static void add(Route*, std::string, std::string, std::string, std::string, std::string);
	static void read_fps(std::string&, ErrorList &);
	static void mark_fps(std::string&, ElapsedTime &);
	static void unmatchedfps_log(std::string&);
	static void datacheck_log(std::string&);

	Datacheck(Route*, std::string, std::string, std::string, std::string, std::string);

	bool match_except_info(std::string*);
	std::string str() const;
};

bool operator < (const Datacheck &, const Datacheck &);
