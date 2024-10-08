#define FMT_HEADER_ONLY
#include "Waypoint.h"
#include "../Datacheck/Datacheck.h"
#include "../ErrorList/ErrorList.h"
#include "../DBFieldLength/DBFieldLength.h"
#include "../HighwaySegment/HighwaySegment.h"
#include "../HighwaySystem/HighwaySystem.h"
#include "../Region/Region.h"
#include "../Route/Route.h"
#include "../../functions/tmstring.h"
#include "../../templates/contains.cpp"
#include <cmath>
#include <cstring>
#include <fmt/format.h>
#define pi 3.141592653589793238

bool sort_root_at_label(Waypoint *w1, Waypoint *w2)
{	return w1->root_at_label() < w2->root_at_label();
}

Waypoint::Waypoint(char *line, Route *rte, ErrorList& el, char*const wptdata)
{	/* initialize object from a .wpt file line */
	#include "validate_label.cpp"
	route = rte;

	// split line into fields
	char* c = strchr(line, ' ');
	if (c){	validate_label(line, c);
		do *c = 0; while (*++c == ' ');
		while (char*d = strchr(c, ' '))
		{	validate_label(c, d);
			do *d = 0; while (*++d == ' ');
			alt_labels.emplace_back(c);
			c = d;
		}
	      }
	label = line;

	// datachecks
	int invalid_line = 0;
	if (label.size() > DBFieldLength::label)
	{	// SINGLE_FIELD_LINE, looks like a URL
		if (!c && !strncmp(label.data(), "http", 4))				// If the "label" is a URL, and the only field...
		{	label = "..."+label.substr(label.size()-DBFieldLength::label+3);// the end is more useful than "http://www.openstreetma..."
			while (label[3] < 0)	label.erase(label.begin()+3);		// Strip any partial multi-byte characters off the beginning
			Datacheck::add(route, label, "", "", "SINGLE_FIELD_LINE", "");
			throw 1;
		}
		// LABEL_TOO_LONG
		size_t slicepoint = DBFieldLength::label-3;				// Prepare a truncated label that will fit in the DB
		while (label[slicepoint-1] < 0) slicepoint--;				// Avoid slicing within a multi-byte character
		std::string excess = label.data()+slicepoint;				// Save excess beyond what fits in DB, to put in info field
		if (excess.size() > DBFieldLength::dcErrValue-3)			// If it's too long for the info field,
		{	excess.assign(excess, 0, DBFieldLength::dcErrValue-6);		// cut it down to what will fit,
			while (excess.back() < 0)	excess.pop_back();		// strip any partial multi-byte characters off the end,
			excess += "...";						// and append "..."
		}
		label.assign(label, 0, slicepoint);					// Now truncate the label itself
		Datacheck::add(route, label+"...", "", "", "LABEL_TOO_LONG", "..."+excess);
		invalid_line = 2;
	}
	// SINGLE_FIELD_LINE, looks like a label
	if (!c)
	{	Datacheck::add(route, label, "", "", "SINGLE_FIELD_LINE", "");
		throw invalid_line | 4;
	}

	// parse URL
	char* latBeg = strstr(c, "lat=")+4;
	char* lonBeg = strstr(c, "lon=")+4;
	if (latBeg == (char*)4 || lonBeg == (char*)4)
	{	bool shortgood = 1;
		char *d=c, *e=c+DBFieldLength::dcErrCode;
		do if (d==e || strchr("\\'\"", *d) || iscntrl(*d))
		   {	shortgood = 0;
			break;
		   }
		while (*++d);
		Datacheck::add(route, label, "", "", "MALFORMED_URL", shortgood ? c : "MISSING_ARG(S)");
		throw invalid_line | 8;
	}
	if (!valid_num_str(latBeg, '&')) {invalid_url(latBeg, "MALFORMED_LAT"); invalid_line |= 16;}
	if (!valid_num_str(lonBeg, '&')) {invalid_url(lonBeg, "MALFORMED_LON"); invalid_line |= 32;}
	if (invalid_line) throw invalid_line;
	lat = atof(latBeg);
	lng = atof(lonBeg);
	is_hidden = label[0] == '+';
	colocated = 0;
}

std::string Waypoint::str()
{	return fmt::format("{} {} ({:.15},{:.15})", route->root, label, lat, lng);
}

bool Waypoint::same_coords(Waypoint *other)
{	/* return if this waypoint is colocated with the other,
	using exact lat,lng match */
	return lat == other->lat && lng == other->lng;
}

bool Waypoint::nearby(Waypoint *other, double tolerance)
{	/* return if this waypoint's coordinates are within the given
	tolerance (in degrees) of the other */
	return fabs(lat - other->lat) < tolerance && fabs(lng - other->lng) < tolerance;
}

double Waypoint::distance_to(Waypoint *other)
{	/* return the distance in miles between this waypoint and another
	including the factor defined by the CHM project to adjust for
	unplotted curves in routes */
	// convert to radians
	double rlat1 = lat * (pi/180);
	double rlng1 = lng * (pi/180);
	double rlat2 = other->lat * (pi/180);
	double rlng2 = other->lng * (pi/180);

	/* original formula
	double ans = acos(cos(rlat1)*cos(rlng1)*cos(rlat2)*cos(rlng2) +\
			  cos(rlat1)*sin(rlng1)*cos(rlat2)*sin(rlng2) +\
			  sin(rlat1)*sin(rlat2)) * 3963.1; // EARTH_RADIUS */

	/* spherical law of cosines formula (same as orig, with some terms factored out or removed via trig identity)
	double ans = acos(cos(rlat1)*cos(rlat2)*cos(rlng2-rlng1)+sin(rlat1)*sin(rlat2)) * 3963.1; /* EARTH_RADIUS */

	/* Vincenty formula
	double ans = 
	 atan (	sqrt(pow(cos(rlat2)*sin(rlng2-rlng1),2)+pow(cos(rlat1)*sin(rlat2)-sin(rlat1)*cos(rlat2)*cos(rlng2-rlng1),2))
		/
		(sin(rlat1)*sin(rlat2)+cos(rlat1)*cos(rlat2)*cos(rlng2-rlng1))
	      ) * 3963.1; /* EARTH_RADIUS */

	// haversine formula
	double ans = asin(sqrt(pow(sin((rlat2-rlat1)/2),2) + cos(rlat1) * cos(rlat2) * pow(sin((rlng2-rlng1)/2),2))) * 7926.2; /* EARTH_DIAMETER */

	return ans * 1.02112; // CHM/TM distance fudge factor to compensate for imprecision of mapping
}

double Waypoint::angle()
{	/* return the angle in degrees formed by the waypoints between the
	line from pred to self and self to succ */
	// convert to radians
	double rlatself = lat * (pi/180);
	double rlngself = lng * (pi/180);
	double rlatpred = this[-1].lat * (pi/180);
	double rlngpred = this[-1].lng * (pi/180);
	double rlatsucc = this[1].lat * (pi/180);
	double rlngsucc = this[1].lng * (pi/180);

	double x0 = cos(rlngpred)*cos(rlatpred);
	double x1 = cos(rlngself)*cos(rlatself);
	double x2 = cos(rlngsucc)*cos(rlatsucc);

	double y0 = sin(rlngpred)*cos(rlatpred);
	double y1 = sin(rlngself)*cos(rlatself);
	double y2 = sin(rlngsucc)*cos(rlatsucc);

	double z0 = sin(rlatpred);
	double z1 = sin(rlatself);
	double z2 = sin(rlatsucc);

	return acos
	(	( (x2-x1)*(x1-x0) + (y2-y1)*(y1-y0) + (z2-z1)*(z1-z0) )
	/ sqrt	(	( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) + (z2-z1)*(z2-z1) )
		*	( (x1-x0)*(x1-x0) + (y1-y0)*(y1-y0) + (z1-z0)*(z1-z0) )
		)
	)
	*180/pi;
}

std::string Waypoint::simple_waypoint_name()
{	/* Failsafe name for a point, simply the string of route name @
	label, concatenated with & characters for colocated points. */
	if (!colocated) return route->list_entry_name() + "@" + label;
	std::string long_label;
	for (Waypoint *w : *colocated)
	  if (w->route->system->active_or_preview())
	  {	if (!long_label.empty()) long_label += "&";
		long_label += w->route->list_entry_name() + "@" + w->label;
	  }
	return long_label;
}

bool Waypoint::is_or_colocated_with_active_or_preview()
{	if (route->system->active_or_preview()) return 1;
	if (colocated)
	  for (Waypoint *w : *colocated)
	    if (w->route->system->active_or_preview()) return 1;
	return 0;
}

std::string Waypoint::root_at_label()
{	return route->root + "@" + label;
}

void Waypoint::nmplogs(std::unordered_set<std::string> &nmpfps, std::ofstream &nmpnmp, std::list<std::string> &nmploglines)
{	if (!near_miss_points.empty())
	{	// sort the near miss points for consistent ordering to facilitate NMP FP marking
		near_miss_points.sort(sort_root_at_label);
		// construct string for nearmisspoints.log & FP matching
		std::string nmpline = str() + " NMP";
		for (Waypoint *other_w : near_miss_points) nmpline += " " + other_w->str();
		// check for string in fp list
		std::unordered_set<std::string>::iterator fpit = nmpfps.find(nmpline);
		bool fp = fpit != nmpfps.end();
		// write lines to tm-master.nmp
		size_t li_count = 0;
		for (Waypoint *other_w : near_miss_points)
		{	bool li = (fabs(lat - other_w->lat) < 0.0000015) && (fabs(lng - other_w->lng) < 0.0000015);
			if (li) li_count++;
			// make sure we only plot once, since the NMP should be listed
			// both ways (other_w in w's list, w in other_w's list)
			if (sort_root_at_label(this, other_w))
			{	char s[51];
				nmpnmp << root_at_label();
				*fmt::format_to(s, " {:.15}", lat)=0; nmpnmp<<s;
				*fmt::format_to(s, " {:.15}", lng)=0; nmpnmp<<s;
				if (fp || li)
				{	nmpnmp << ' ';
					if (fp) nmpnmp << "FP";
					if (li) nmpnmp << "LI";
				}
				nmpnmp << '\n';

				nmpnmp << other_w->root_at_label();
				*fmt::format_to(s, " {:.15}", other_w->lat)=0; nmpnmp<<s;
				*fmt::format_to(s, " {:.15}", other_w->lng)=0; nmpnmp<<s;
				if (fp || li)
				{	nmpnmp << ' ';
					if (fp) nmpnmp << "FP";
					if (li) nmpnmp << "LI";
				}
				nmpnmp << '\n';
			}
		}
		// indicate if this was in the FP list or if it's off by exact amt
		// so looks like it's intentional, and detach near_miss_points list
		// so it doesn't get a rewrite in nmp_merged WPT files
		if (li_count)
		{	if ( li_count == std::distance(near_miss_points.begin(), near_miss_points.end()) )
				nmpline += " [LOOKS INTENTIONAL]";
			else	nmpline += " [SOME LOOK INTENTIONAL]";
			near_miss_points.clear();
		}
		if (fp)
		{	nmpfps.erase(fpit);
			nmpline += " [MARKED FP]";
			near_miss_points.clear();
		}
		nmploglines.push_back(nmpline);
	}
}


Waypoint* Waypoint::hashpoint()
{	// return a canonical waypoint for graph vertex hashtable lookup
	if (!colocated) return this;
	return colocated->front();
}

bool Waypoint::label_references_route(Route *r)
{	std::string no_abbrev = r->name_no_abbrev();
	if ( strncmp(label.data(), no_abbrev.data(), no_abbrev.size()) )
		return 0;
	const char* c = label.data() + no_abbrev.size();
	if (*c == 0 || *c == '_')
		return 1;
	if ( strncmp(c, r->abbrev.data(), r->abbrev.size()) )
	{	//if (*c == '/') Datacheck::add(route, label, "", "", "UNEXPECTED_DESIGNATION", c+1);
		return 0;
	}
	c += r->abbrev.size();
	if (*c == 0 || *c == '_')
		return 1;
	//if (*c == '/') Datacheck::add(route, label, "", "", "UNEXPECTED_DESIGNATION", c+1);
	return 0;
}

/* Datacheck helper functions */

Route* Waypoint::coloc_same_number(const char* digits)
{	if (colocated)
	  for (Waypoint* w : *colocated)
	  {	if (w == this) continue;
		const char* d = w->route->route.data();
		while (!isdigit(*d) && *d) ++d;
		std::string other(d);
		if (w->route->banner[0] == '-') other += w->route->banner;
		if (digits == other) return w->route;
	  }
	return 0;
}

Route* Waypoint::coloc_same_designation(const std::string& rte)
{	if (colocated)
	  for (Waypoint* w : *colocated)
	    if ( w != this && rte == (w->route->banner[0] == '-' ? w->route->route : w->route->name_no_abbrev()) )
	      return w->route;
	return 0;
}

Route* Waypoint::self_intersection()
{	if (colocated)
	  for (Waypoint* w : *colocated)
	    if (w != this && w->route == route)
	      return w->route;
	return 0;
}

bool Waypoint::banner_after_slash(const char* slash)
{	// return whether colocated with a bannered route of same designation whose banner
	// is contained in this waypoint's label after a slash, before underscore if exists
	if (colocated)
	  for (Waypoint* w : *colocated)
	    if (w != this && w->route->banner.size() && w->route->route == route->route && strdstr(slash+1, w->route->banner.data(), '_'))
	      return 1;
	return 0;
}

Route* Waypoint::coloc_banner_matches_abbrev()
{	// return whether colocated with a bannered route of same designation whose banner
	// matches this route's abbrev
	if (colocated)
	  for (Waypoint* w : *colocated)
	    if (w != this && w->route->route == route->route && w->route->banner == route->abbrev)
	      return w->route;
	return 0;
}

inline void add_to_adjacent(std::vector<HighwaySegment*>& adjacent, HighwaySegment& s)
{	// Store a unique-location identifier.
	// Let's use the address of singleton segments, and the front of the
	// concurrency list for concurrent segments as it's shared by all of them.
	HighwaySegment* a = s.concurrent ? s.concurrent->front() : &s;
	if (!contains(adjacent, a))
		adjacent.push_back(a);
}

/*DEBUG
inline std::string cat_seg(std::vector<HighwaySegment*>& v)
{	std::string info;
	for (HighwaySegment* s : v)
	{	info += "\n ";
		if (s->concurrent)
		    for (HighwaySegment* c : *s->concurrent)
			info.append(" [").append(c->str()).append("]");
		else	info.append(" [").append(s->str()).append("]");
	}
	return info;
}//*/

/* Datacheck */

void Waypoint::hidden_junction()
{	// we will have already checked for visibility before calling this function
	if (!colocated || this != colocated->front()) return;
	std::vector<HighwaySegment*> adjacent;	// Make a list of unique adjacent locations
	for (Waypoint* w : *colocated)	// before & after every point on this colocation list
	{	// Kill 2 birds with 1 stone:
		// 1.	As with graph vertices where it originated, all colocated points must be
		//	hidden for the junction to count as hidden for purposes of this datacheck.
		//	If we find a visible point, return.
		// 2.	If we do find one, there's our VISIBLE_HIDDEN_COLOC error, so return that.
		if (!w->is_hidden)
			return Datacheck::add(w->route, w->label, "", "", "VISIBLE_HIDDEN_COLOC", root_at_label());
		size_t index = w - w->route->points.data;
		if (index)					      // unless 1st point in route,
			add_to_adjacent(adjacent, w->route->segments[index-1]);	// add prev segment
		if (index != w->route->points.size - 1)		     // unless last point in route,
			add_to_adjacent(adjacent, w->route->segments[index]);	// add next segment
	}
	if (adjacent.size() > 2 || adjacent.size() == 2 && colocated && !adjacent[0]->same_vis_routes(adjacent[1]))
		Datacheck::add( route, label, "", "", "HIDDEN_JUNCTION", std::to_string(adjacent.size())/*+cat_seg(adjacent)*/ );
}

void Waypoint::invalid_url(const char* const cstr, const char* const errcode)
{	std::string str(cstr, strcspn(cstr, "&"));
	if (str.size() > DBFieldLength::dcErrValue)
	{	str.assign(str, 0, DBFieldLength::dcErrValue-3);
		while (str.back() < 0)	str.pop_back();
		str += "...";
	}
	Datacheck::add(route, label, "", "", errcode, str);
}

void Waypoint::out_of_bounds()
{	// out-of-bounds coords
	if (lat > 90 || lat < -90 || lng > 180 || lng < -180)
	  Datacheck::add(route, label, "", "", "OUT_OF_BOUNDS", fmt::format("({:.15},{:.15})", lat, lng));
}

/* checks for visible points */

void Waypoint::bus_with_i()
{	// look for I-xx with Bus instead of BL or BS
	const char *c = label.data();
	if (*c == '*') c++;
	if (*c++ != 'I' || *c++ != '-') return;
	if (*c < '0' || *c > '9') return;
	while (*c >= '0' && *c <= '9') c++;
	if ( *c == 'E' || *c == 'W' || *c == 'C' || *c == 'N' || *c == 'S'
	  || *c == 'e' || *c == 'w' || *c == 'c' || *c == 'n' || *c == 's' ) c++;
	if ( (*c == 'B' || *c == 'b')
	  && (*(c+1) == 'u' || *(c+1) == 'U')
	  && (*(c+2) == 's' || *(c+2) == 'S') )
		Datacheck::add(route, label, "", "", "BUS_WITH_I", "");
}

void Waypoint::interstate_no_hyphen()
{	const char *c = label[0] == '*' ? label.data()+1 : label.data();
	if (c[0] == 'T' && c[1] == 'o') c += 2;
	if (c[0] == 'I' && isdigit(c[1]))
	  Datacheck::add(route, label, "", "", "INTERSTATE_NO_HYPHEN", "");
}

void Waypoint::label_invalid_ends()
{	// look for labels with invalid first or final characters
	const char *c = label.data();
	while (*c == '*') c++;
	if (*c == '_' || *c == '/' || *c == '(')
		Datacheck::add(route, label, "", "", "INVALID_FIRST_CHAR", std::string(1, *c));
	if (label.back() == '_' || label.back() == '/')
		Datacheck::add(route, label, "", "", "INVALID_FINAL_CHAR", std::string(1, label.back()));
}

void Waypoint::label_looks_hidden()
{	// look for labels that look like hidden waypoints but which aren't hidden
	if (label.size() != 7)			return;
	if (label[0] != 'X')			return;
	if (label[1] < '0' || label[1] > '9')	return;
	if (label[2] < '0' || label[2] > '9')	return;
	if (label[3] < '0' || label[3] > '9')	return;
	if (label[4] < '0' || label[4] > '9')	return;
	if (label[5] < '0' || label[5] > '9')	return;
	if (label[6] < '0' || label[6] > '9')	return;
	Datacheck::add(route, label, "", "", "LABEL_LOOKS_HIDDEN", "");
}

void Waypoint::label_lowercase()
{	if (islower(label[label[0]=='*'])) Datacheck::add(route, label, "", "", "LABEL_LOWERCASE", "");
}

void Waypoint::label_parens()
{	// look for parenthesis balance in label
	int parens = 0;
	const char *left = 0;
	const char* right = 0;
	for (const char *c = label.data(); *c; c++)
	{	if (*c == '(')
		     {	if (left)
			{	Datacheck::add(route, label, "", "", "LABEL_PARENS", "");
				return;
			}
			left = c;
			parens++;
		     }
		else if	(*c == ')')
		     {	right = c;
			parens--;
		     }
	}
	if (parens || right < left)
		Datacheck::add(route, label, "", "", "LABEL_PARENS", "");
}

void Waypoint::label_selfref()
{	// "label references own route"
	#define FLAG(SUBTYPE) Datacheck::add(route, label, "", "", "LABEL_SELFREF", SUBTYPE)
	const char* l = label.data() + (label[0] == '*');
	std::string rte = route->banner[0] == '-' ? route->route : route->name_no_abbrev();
	// first check for number match after a slash, if there is one
	const char* slash = strchr(l, '/');
	if (slash++ && strncmp(l, "Old", 3))
	{	const char* digits = rte.data();
		while (!isdigit(*digits) && *digits) ++digits;
		if (	digits != rte.data()+rte.size()
		     &&	!strdcmp(slash, digits, '_') && !coloc_same_number(digits)
		     ||	!strdcmp(slash, rte.data(), '_') && !coloc_same_designation(rte)
		   )				return	FLAG("NO_COLOC");
	}
	// now the remaining checks
	if ( strncmp(l, rte.data(), rte.size()) ) return;
	const char *c = l+rte.size();
	switch(*c)
	{   case 0:   {	Route* r = coloc_same_designation(rte);
			if (!r)				FLAG("NO_COLOC");
			else if (r == route)		FLAG("NO_SUFFIX");
			else if (r->abbrev.size())	FLAG("NO_ABBREV");
		      }	return;
	    case '_': {	if (*++c == 'U')
			{	while (isdigit(*++c));
				if (!*c) return;
			}
			Route* r = coloc_same_designation(rte);
			if (!r)				FLAG("NO_COLOC");
			else if (r->abbrev.size())	FLAG("NO_ABBREV");
		      }	return;
	    case '/':	if ((route->banner.size() && route->banner[0] != '-' || !banner_after_slash(slash)) && !coloc_same_designation(rte))
							FLAG("NO_COLOC");
			return;
	    default:	if (	route->abbrev.size()
			     &&	!strncmp(c, route->abbrev.data(), route->abbrev.size())
			     &&	(!self_intersection() || !c[route->abbrev.size()])
			     && !coloc_banner_matches_abbrev()
			   )				FLAG(strchr("_/", c[route->abbrev.size()]) ? "TRUE_ERROR" : "FULL_MATCH");
	}
	#undef FLAG
}

void Waypoint::label_slashes(const char *slash)
{	// look for too many slashes in label
	if (slash && strchr(slash+1, '/'))
		Datacheck::add(route, label, "", "", "LABEL_SLASHES", "");
}

void Waypoint::lacks_generic()
{	// label lacks generic highway type
	const char* c = label[0] == '*' ? label.data()+1 : label.data();
	if ( (*c == 'O' || *c == 'o')
	  && (*(c+1) == 'l' || *(c+1) == 'L')
	  && (*(c+2) == 'd' || *(c+2) == 'D')
	  &&  *(c+3) >= '0' && *(c+3) <= '9')
		Datacheck::add(route, label, "", "", "LACKS_GENERIC", "");
}

void Waypoint::underscore_datachecks(const char *slash)
{	const char *underscore = strchr(label.data(), '_');
	if (underscore)
	{	// look for too many underscores in label
		if (strchr(underscore+1, '_'))
			Datacheck::add(route, label, "", "", "LABEL_UNDERSCORES", "");
		// look for too many characters after underscore in label
		if (label.data()+label.size() > underscore+4)
		    if (label.back() > 'Z' || label.back() < 'A' || label.data()+label.size() > underscore+5)
			Datacheck::add(route, label, "", "", "LONG_UNDERSCORE", "");
		// look for labels with a slash after an underscore
		if (slash > underscore)
			Datacheck::add(route, label, "", "", "NONTERMINAL_UNDERSCORE", "");
		// look for suffix starting with lowercase letter
		if (islower(underscore[1]))
			Datacheck::add(route, label, "", "", "LOWERCASE_SUFFIX", "");
	}
}

void Waypoint::us_letter()
{	// look for USxxxA but not USxxxAlt, B/Bus/Byp
	const char* c = label[0] == '*' ? label.data()+1 : label.data();
	if (*c++ != 'U' || *c++ != 'S')	return;
	if (*c    < '0' || *c++  > '9')	return;
	while (*c >= '0' && *c <= '9')	c++;
	if (*c    < 'A' || *c++  > 'B')	return;
	if (*c == 0 || *c == '/' || *c == '_' || *c == '(')
		Datacheck::add(route, label, "", "", "US_LETTER", "");
	// is it followed by a city abbrev?
	else if (*c >= 'A' && *c++ <= 'Z'
	      && *c >= 'a' && *c++ <= 'z'
	      && *c >= 'a' && *c++ <= 'z'
	      && *c == 0 || *c == '/' || *c == '_' || *c == '(')
		Datacheck::add(route, label, "", "", "US_LETTER", "");
}

void Waypoint::visible_distance(double &vis_dist, Waypoint *&last_visible)
{	// complete visible distance check, omit report for active
	// systems to reduce clutter
	if (vis_dist > 10 && !route->system->active())
	  Datacheck::add(route, last_visible->label, label, "", "VISIBLE_DISTANCE", fmt::format("{:.2f}", vis_dist));
	last_visible = this;
	vis_dist = 0;
}

#undef pi
