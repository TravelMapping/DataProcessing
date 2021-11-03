#include "Waypoint.h"
#include "../Datacheck/Datacheck.h"
#include "../DBFieldLength/DBFieldLength.h"
#include "../HighwaySystem/HighwaySystem.h"
#include "../Route/Route.h"
#include "../../functions/valid_num_str.h"
#include <cmath>
#include <cstring>
#define pi 3.141592653589793238

bool sort_root_at_label(Waypoint *w1, Waypoint *w2)
{	return w1->root_at_label() < w2->root_at_label();
}

Waypoint::Waypoint(char *line, Route *rte)
{	/* initialize object from a .wpt file line */
	route = rte;

	// split line into fields
	char* c = strchr(line, ' ');
	if (c){	do *c = 0; while (*++c == ' ');
		// alt labels & URL
		for (size_t spn = 0; *c; c += spn)
		{	for (spn = strcspn(c, " "); c[spn] == ' '; spn++) c[spn] = 0;
			alt_labels.emplace_back(c);
		}
	      }
	label = line;

	// datachecks
	bool valid_line = 1;
	if (label.size() > DBFieldLength::label)
	{	// SINGLE_FIELD_LINE, looks like a URL
		if (alt_labels.empty() && !strncmp(label.data(), "http", 4))		// If the "label" is a URL, and the only field...
		{	label = "..."+label.substr(label.size()-DBFieldLength::label+3);// the end is more useful than "http://www.openstreetma..."
			while (label.front() < 0)	label.erase(label.begin());	// Strip any partial multi-byte characters off the beginning
			Datacheck::add(route, label, "", "", "SINGLE_FIELD_LINE", "");
			lat = 0;	lng = 0;	return;
		}
		// LABEL_TOO_LONG
		size_t slicepoint = DBFieldLength::label-3;				// Prepare a truncated label that will fit in the DB
		while (label[slicepoint-1] < 0) slicepoint--;				// Avoid slicing within a multi-byte character
		std::string excess = label.data()+slicepoint;				// Save excess beyond what fits in DB, to put in info field
		if (excess.size() > DBFieldLength::dcErrValue-3)			// If it's too long for the info field,
		{	excess = excess.substr(0, DBFieldLength::dcErrValue-6);		// cut it down to what will fit,
			while (excess.back() < 0)	excess.erase(excess.end()-1);	// strip any partial multi-byte characters off the end,
			excess += "...";						// and append "..."
		}
		label = label.assign(label, 0, slicepoint);				// Now truncate the label itself
		Datacheck::add(route, label+"...", "", "", "LABEL_TOO_LONG", "..."+excess);
		valid_line = 0;
	}
	// SINGLE_FIELD_LINE, looks like a label
	if (alt_labels.empty())
	{	Datacheck::add(route, label, "", "", "SINGLE_FIELD_LINE", "");
		lat = 0;	lng = 0;	return;
	}

	// parse URL
	std::string& URL = alt_labels.back();	// last field is actually the URL, not a label.
	size_t latBeg = URL.find("lat=")+4;
	size_t lonBeg = URL.find("lon=")+4;
	if (latBeg == 3 || lonBeg == 3)
	{	Datacheck::add(route, label, "", "", "MALFORMED_URL", "MISSING_ARG(S)");
		lat = 0;	lng = 0;	return;
	}
	if (!valid_num_str(URL.data()+latBeg, '&'))
	{	size_t ampersand = URL.find('&', latBeg);
		std::string lat_string = (ampersand == -1) ? URL.data()+latBeg : URL.substr(latBeg, ampersand-latBeg);
		if (lat_string.size() > DBFieldLength::dcErrValue)
		{	lat_string = lat_string.substr(0, DBFieldLength::dcErrValue-3);
			while (lat_string.back() < 0)	lat_string.erase(lat_string.end()-1);
			lat_string += "...";
		}
		Datacheck::add(route, label, "", "", "MALFORMED_LAT", lat_string);
		valid_line = 0;
	}
	if (!valid_num_str(URL.data()+lonBeg, '&'))
	{	size_t ampersand = URL.find('&', lonBeg);
		std::string lng_string = (ampersand == -1) ? URL.data()+lonBeg : URL.substr(lonBeg, ampersand-lonBeg);
		if (lng_string.size() > DBFieldLength::dcErrValue)
		{	lng_string = lng_string.substr(0, DBFieldLength::dcErrValue-3);
			while (lng_string.back() < 0)	lng_string.erase(lng_string.end()-1);
			lng_string += "...";
		}
		Datacheck::add(route, label, "", "", "MALFORMED_LON", lng_string);
		valid_line = 0;
	}
	if (valid_line)
	     {	lat = strtod(&URL[latBeg], 0);
		lng = strtod(&URL[lonBeg], 0);
		alt_labels.pop_back(); // remove URL
		is_hidden = label[0] == '+';
		colocated = 0;
	     }
	else {	lat = 0;
		lng = 0;
	     }
}

std::string Waypoint::str()
{	std::string ans = route->root + " " + label;
	char coordstr[51];
	sprintf(coordstr, "%.15g", lat);
	if (!strchr(coordstr, '.')) strcat(coordstr, ".0"); // add single trailing zero to ints for compatibility with Python
	ans += " (";
	ans += coordstr;
	ans += ',';
	sprintf(coordstr, "%.15g", lng);
	if (!strchr(coordstr, '.')) strcat(coordstr, ".0"); // add single trailing zero to ints for compatibility with Python
	ans += coordstr;
	return ans + ')';
}

std::string Waypoint::csv_line(unsigned int id)
{	/* return csv line to insert into a table */
	char fstr[64];
	sprintf(fstr, "','%.15g','%.15g','", lat, lng);
	return "'" + std::to_string(id) + "','" + label + fstr + route->root + "'";
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

double Waypoint::angle(Waypoint *pred, Waypoint *succ)
{	/* return the angle in degrees formed by the waypoints between the
	line from pred to self and self to succ */
	// convert to radians
	double rlatself = lat * (pi/180);
	double rlngself = lng * (pi/180);
	double rlatpred = pred->lat * (pi/180);
	double rlngpred = pred->lng * (pi/180);
	double rlatsucc = succ->lat * (pi/180);
	double rlngsucc = succ->lng * (pi/180);

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
			{	char coordstr[51];

				nmpnmp << root_at_label();
				sprintf(coordstr, " %.15g", lat);
				if (!strchr(coordstr, '.')) strcat(coordstr, ".0"); // add single trailing zero to ints for compatibility with Python
				nmpnmp << coordstr;
				sprintf(coordstr, " %.15g", lng);
				if (!strchr(coordstr, '.')) strcat(coordstr, ".0"); // add single trailing zero to ints for compatibility with Python
				nmpnmp << coordstr;
				if (fp || li)
				{	nmpnmp << ' ';
					if (fp) nmpnmp << "FP";
					if (li) nmpnmp << "LI";
				}
				nmpnmp << '\n';

				nmpnmp << other_w->root_at_label();
				sprintf(coordstr, " %.15g", other_w->lat);
				if (!strchr(coordstr, '.')) strcat(coordstr, ".0"); // add single trailing zero to ints for compatibility with Python
				nmpnmp << coordstr;
				sprintf(coordstr, " %.15g", other_w->lng);
				if (!strchr(coordstr, '.')) strcat(coordstr, ".0"); // add single trailing zero to ints for compatibility with Python
				nmpnmp << coordstr;
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
	if (label[no_abbrev.size()] == 0 || label[no_abbrev.size()] == '_')
		return 1;
	if ( strncmp(label.data()+no_abbrev.size(), r->abbrev.data(), r->abbrev.size()) )
	{	/*if (label[no_abbrev.size()] == '/')
			Datacheck::add(route, label, "", "", "UNEXPECTED_DESIGNATION", label.data()+no_abbrev.size()+1);//*/
		return 0;
	}
	if (label[no_abbrev.size() + r->abbrev.size()] == 0 || label[no_abbrev.size() + r->abbrev.size()] == '_')
		return 1;
	/*if (label[no_abbrev.size() + r->abbrev.size()] == '/')
		Datacheck::add(route, label, "", "", "UNEXPECTED_DESIGNATION", label.data()+no_abbrev.size()+r->abbrev.size()+1);//*/
	return 0;
}

/* Datacheck */

void Waypoint::distance_update(char *fstr, double &vis_dist, Waypoint *prev_w)
{	// visible distance update, and last segment length check
	double last_distance = distance_to(prev_w);
	vis_dist += last_distance;
	if (last_distance > 20)
	{	sprintf(fstr, "%.2f", last_distance);
		Datacheck::add(route, prev_w->label, label, "", "LONG_SEGMENT", fstr);
	}
}

void Waypoint::label_invalid_char()
{	// look for labels with invalid characters
	if (label == "*")
		  Datacheck::add(route, label, "", "", "LABEL_INVALID_CHAR", "");
	else for (const char *c = label.data(); *c; c++)
		if ((*c == 42 || *c == 43) && c > label.data()
		 || (*c < 40)	|| (*c == 44)	|| (*c > 57 && *c < 65)
		 || (*c == 96)	|| (*c > 122)	|| (*c > 90 && *c < 95))
		{	if (!strncmp(label.data(), "\xEF\xBB\xBF", 3))
				Datacheck::add(route, label, "", "", "LABEL_INVALID_CHAR", "UTF-8 BOM");
			else	Datacheck::add(route, label, "", "", "LABEL_INVALID_CHAR", "");
			break;
		}
	for (std::string& lbl : alt_labels)
	  if (lbl == "*")
		  Datacheck::add(route, lbl, "", "", "LABEL_INVALID_CHAR", "");
	  else for (const char *c = lbl.data(); *c; c++)
		if (*c == '+' && c > lbl.data() || *c == '*' && (c > lbl.data()+1 || lbl[0] != '+')
		 || (*c < 40)	|| (*c == 44)	|| (*c > 57 && *c < 65)
		 || (*c == 96)	|| (*c > 122)	|| (*c > 90 && *c < 95))
		{	Datacheck::add(route, lbl, "", "", "LABEL_INVALID_CHAR", "");
			break;
		}
}

void Waypoint::out_of_bounds(char *fstr)
{	// out-of-bounds coords
	if (lat > 90 || lat < -90 || lng > 180 || lng < -180)
	{	sprintf(fstr, "(%.15g,%.15g)", lat, lng);
		Datacheck::add(route, label, "", "", "OUT_OF_BOUNDS", fstr);
	}
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

void Waypoint::label_selfref(const char *slash)
{	// looking for the route within the label
	// partially complete "references own route" -- too many FP
	// first check for number match after a slash, if there is one
	if (slash)
	{	char* digit_starts = isdigit(route->route.back())
		? &route->route.back()
		: isdigit(route->route[route->route.size()-2]) && isalpha(route->route.back()) ? &route->route.back()-1 : 0;
		if (digit_starts)
		{	while (digit_starts > route->route.data() && isdigit(digit_starts[-1])) digit_starts--;
			if (!strcmp(slash+1, digit_starts) || !strcmp(slash+1, route->route.data()))
			{	Datacheck::add(route, label, "", "", "LABEL_SELFREF", "");
				return;
			}
			const char *underscore = strchr(slash+1, '_');
			if (underscore)
			{	std::string postslash(slash+1, underscore-slash-1);
				if (postslash == digit_starts || postslash == route->route)
				{	Datacheck::add(route, label, "", "", "LABEL_SELFREF", "");
					return;
				}
			}
		}
	}
	// now the remaining checks
	std::string rte_ban = route->route + route->banner;
	if ( strncmp(label.data(), rte_ban.data(), rte_ban.size()) ) return;
	const char *c = label.data()+rte_ban.size();
	if (*c == 0 || *c == '/')
		Datacheck::add(route, label, "", "", "LABEL_SELFREF", "");
	else if (*c == '_')
		if (*++c != 'U')
			Datacheck::add(route, label, "", "", "LABEL_SELFREF", "");
		else {	while (isdigit(*++c));
			if (*c) Datacheck::add(route, label, "", "", "LABEL_SELFREF", "");
		     }
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

void Waypoint::visible_distance(char *fstr, double &vis_dist, Waypoint *&last_visible)
{	// complete visible distance check, omit report for active
	// systems to reduce clutter
	if (vis_dist > 10 && !route->system->active())
	{	sprintf(fstr, "%.2f", vis_dist);
		Datacheck::add(route, last_visible->label, label, "", "VISIBLE_DISTANCE", fstr);
	}
	last_visible = this;
	vis_dist = 0;
}

#undef pi
