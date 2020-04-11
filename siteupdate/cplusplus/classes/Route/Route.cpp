std::mutex Route::liu_mtx;
std::mutex Route::ual_mtx;
std::mutex Route::awf_mtx;

Route::Route(std::string &line, HighwaySystem *sys, ErrorList &el, std::unordered_map<std::string, Region*> &region_hash)
{	/* initialize object from a .csv file line,
	but do not yet read in waypoint file */
	con_route = 0;
	mileage = 0;
	rootOrder = -1; // order within connected route
	region = 0;	// if this stays 0, setup has failed due to bad .csv data

	// parse chopped routes csv line
	size_t NumFields = 8;
	std::string sys_str, rg_str, arn_str;
	std::string* fields[8] = {&sys_str, &rg_str, &route, &banner, &abbrev, &city, &root, &arn_str};
	split(line, fields, NumFields, ';');
	if (NumFields != 8)
	{	el.add_error("Could not parse " + sys->systemname
			   + ".csv line: [" + line + "], expected 8 fields, found " + std::to_string(NumFields));
		root.clear(); // in case it was filled by a line with 7 or 9+ fields
		return;
	}
	// system
	system = sys;
	if (system->systemname != sys_str)
		el.add_error("System mismatch parsing " + system->systemname
			   + ".csv line [" + line + "], expected " + system->systemname);
	// region
	try {	region = region_hash.at(rg_str);
	    }
	catch (const std::out_of_range& oor)
	    {	el.add_error("Unrecognized region in " + system->systemname
			   + ".csv line: " + line);
		return;
	    }
	// route
	if (route.size() > DBFieldLength::route)
		el.add_error("Route > " + std::to_string(DBFieldLength::route)
			   + " bytes in " + system->systemname + ".csv line: " + line);
	// banner
	if (banner.size() > DBFieldLength::banner)
		el.add_error("Banner > " + std::to_string(DBFieldLength::banner)
			   + " bytes in " + system->systemname + ".csv line: " + line);
	// abbrev
	if (abbrev.size() > DBFieldLength::abbrev)
		el.add_error("Abbrev > " + std::to_string(DBFieldLength::abbrev)
			   + " bytes in " + system->systemname + ".csv line: " + line);
	// city
	if (city.size() > DBFieldLength::city)
		el.add_error("City > " + std::to_string(DBFieldLength::city)
			   + " bytes in " + system->systemname + ".csv line: " + line);
	// root
	if (root.size() > DBFieldLength::root)
		el.add_error("Root > " + std::to_string(DBFieldLength::root)
			   + " bytes in " + system->systemname + ".csv line: " + line);
	// alt_route_names
	size_t l = 0;
	for (size_t r = 0; r != -1; l = r+1)
	{	r = arn_str.find(',', l);
		alt_route_names.emplace_back(arn_str, l, r-l);
	}
}

std::string Route::str()
{	/* printable version of the object */
	return root + " (" + std::to_string(point_list.size()) + " total points)";
}

#include "read_wpt.cpp"

void Route::print_route()
{	for (Waypoint *point : point_list)
		std::cout << point->str() << std::endl;
}

HighwaySegment* Route::find_segment_by_waypoints(Waypoint *w1, Waypoint *w2)
{	for (HighwaySegment *s : segment_list)
	  if (s->waypoint1 == w1 && s->waypoint2 == w2 || s->waypoint1 == w2 && s->waypoint2 == w1)
	    return s;
	return 0;
}

std::string Route::chopped_rtes_line()
{	/* return a chopped routes system csv line, for debug purposes */
	std::string line = system->systemname + ';' + region->code + ';' + route + ';' \
			 + banner + ';' + abbrev + ';' + city + ';' + root + ';';
	if (!alt_route_names.empty())
	{	line += alt_route_names[0];
		for (size_t i = 1; i < alt_route_names.size(); i++)
			line += ',' + alt_route_names[i];
	}
	return line;
}

std::string Route::csv_line()
{	/* return csv line to insert into a table */
	// note: alt_route_names does not need to be in the db since
	// list preprocessing uses alt or canonical and no longer cares
	std::string line = "'" + system->systemname + "','" + region->code + "','" + route + "','" + banner
			 + "','" + abbrev + "','" + double_quotes(city) + "','" + root + "','";
	char mstr[51];
	sprintf(mstr, "%.17g", mileage);
	if (!strchr(mstr, '.')) strcat(mstr, ".0"); // add single trailing zero to ints for compatibility with Python
	line += mstr;
	line += "','" + std::to_string(rootOrder) + "'";
	return line;
}

std::string Route::readable_name()
{	/* return a string for a human-readable route name */
	return region->code + " " + route + banner + abbrev;
}

std::string Route::list_entry_name()
{	/* return a string for a human-readable route name in the
	format expected in traveler list files */
	return route + banner + abbrev;
}

std::string Route::name_no_abbrev()
{	/* return a string for a human-readable route name in the
	format that might be encountered for intersecting route
	labels, where the abbrev field is often omitted */
	return route + banner;
}

double Route::clinched_by_traveler(TravelerList *t)
{	double miles = 0;
	for (HighwaySegment *s : segment_list)
	{	std::unordered_set<TravelerList*>::iterator t_found = s->clinched_by.find(t);
		if (t_found != s->clinched_by.end()) miles += s->length;
	}
	return miles;
}

std::string Route::list_line(int beg, int end)
{	/* Return a .list file line from (beg) to (end),
	these being indices to the point_list vector.
	These values can be "out-of-bounds" when getting lines
	for connected routes. If so, truncate or return "". */
	if (beg >= int(point_list.size()) || end <= 0) return "";
	if (end >= int(point_list.size())) end = point_list.size()-1;
	if (beg < 0) beg = 0;
	return readable_name() + " " + point_list[beg]->label + " " + point_list[end]->label;
}

void Route::write_nmp_merged(std::string filename)
{	mkdir(filename.data(), 0777);
	filename += "/" + system->systemname;
	mkdir(filename.data(), 0777);
	filename += "/" + root + ".wpt";
	std::ofstream wptfile(filename);
	char fstr[12];
	for (Waypoint *w : point_list)
	{	wptfile << w->label << ' ';
		for (std::string &a : w->alt_labels) wptfile << a << ' ';
		if (w->near_miss_points.empty())
		     {	wptfile << "http://www.openstreetmap.org/?lat=";
			sprintf(fstr, "%.6f", w->lat);
			wptfile << fstr << "&lon=";
			sprintf(fstr, "%.6f", w->lng);
			wptfile << fstr << '\n';
		     }
		else {	// for now, arbitrarily choose the northernmost
			// latitude and easternmost longitude values in the
			// list and denote a "merged" point with the https
			double lat = w->lat;
			double lng = w->lng;
			for (Waypoint *other_w : w->near_miss_points)
			{	if (other_w->lat > lat)	lat = other_w->lat;
				if (other_w->lng > lng)	lng = other_w->lng;
			}
			wptfile << "https://www.openstreetmap.org/?lat=";
			sprintf(fstr, "%.6f", lat);
			wptfile << fstr << "&lon=";
			sprintf(fstr, "%.6f", lng);
			wptfile << fstr << '\n';
			w->near_miss_points.clear();
		     }
	}
	wptfile.close();
}

Route *route_by_root(std::string root, std::list<Route> &route_list)
{	for (std::list<Route>::iterator r = route_list.begin(); r != route_list.end(); r++)
		if (r->root == root) return &*r;
	return 0;
}
