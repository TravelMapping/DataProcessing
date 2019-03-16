std::mutex Route::liu_mtx;
std::mutex Route::ual_mtx;
std::mutex Route::awf_mtx;

Route::Route(std::string &line, HighwaySystem *sys, ErrorList &el, std::list<Region> &all_regions)
{	/* initialize object from a .csv file line,
	but do not yet read in waypoint file */
	mileage = 0;
	rootOrder = -1;  // order within connected route
	if (line.back() == 0x0D) line.erase(line.end()-1);	// trim DOS newlines

	// system
	size_t left = line.find(';');
	if (left == std::string::npos)
	{	el.add_error("Could not parse " + system->systemname + ".csv line: [" + line + "], expected 8 fields, found 1");
		return;
	}
	system = sys;
	if (system->systemname != line.substr(0,left))
	    {	el.add_error("System mismatch parsing " + system->systemname + ".csv line [" + line + "], expected " + system->systemname);
		return;
	    }

	// region
	size_t right = line.find(';', left+1);
	if (right == std::string::npos)
	{	el.add_error("Could not parse " + system->systemname + ".csv line: [" + line + "], expected 8 fields, found 2");
		return;
	}
	region = region_by_code(line.substr(left+1, right-left-1), all_regions);
	if (!region)
	    {	el.add_error("Unrecognized region in " + system->systemname + ".csv line: [" + line + "], " + line.substr(left+1, right-left-1));
		return;
	    }

	// route
	left = right;
	right = line.find(';', left+1);
	if (right == std::string::npos)
	{	el.add_error("Could not parse " + system->systemname + ".csv line: [" + line + "], expected 8 fields, found 3");
		return;
	}
	route = line.substr(left+1, right-left-1);

	// banner
	left = right;
	right = line.find(';', left+1);
	if (right == std::string::npos)
	{	el.add_error("Could not parse " + system->systemname + ".csv line: [" + line + "], expected 8 fields, found 4");
		return;
	}
	banner = line.substr(left+1, right-left-1);
	if (banner.size() > 6)
	    {	el.add_error("Banner >6 characters in " + system->systemname + ".csv line: [" + line + "], " + banner);
		return;
	    }

	// abbrev
	left = right;
	right = line.find(';', left+1);
	if (right == std::string::npos)
	{	el.add_error("Could not parse " + system->systemname + ".csv line: [" + line + "], expected 8 fields, found 5");
		return;
	}
	abbrev = line.substr(left+1, right-left-1);
	if (abbrev.size() > 3)
	    {	el.add_error("Abbrev >3 characters in " + system->systemname + ".csv line: [" + line + "], " + abbrev);
		return;
	    }

	// city
	left = right;
	right = line.find(';', left+1);
	if (right == std::string::npos)
	{	el.add_error("Could not parse " + system->systemname + ".csv line: [" + line + "], expected 8 fields, found 6");
		return;
	}
	city = line.substr(left+1, right-left-1);

	// root
	left = right;
	right = line.find(';', left+1);
	if (right == std::string::npos)
	{	el.add_error("Could not parse " + system->systemname + ".csv line: [" + line + "], expected 8 fields, found 7");
		return;
	}
	root = line.substr(left+1, right-left-1);

	// alt_route_names
	left = right;
	right = line.find(';', left+1);
	if (right != std::string::npos)
	{	el.add_error("Could not parse " + system->systemname + ".csv line: [" + line + "], expected 8 fields, found more");
		return;
	}
	char *arnstr = new char[line.size()-left];
	strcpy(arnstr, line.substr(left+1).data());
	for (char *token = strtok(arnstr, ","); token; token = strtok(0, ","))
		alt_route_names.push_back(token);
	delete[] arnstr;
	//yDEBUG*/ std::cout << "returning from Route ctor" << endl;
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
	sprintf(mstr, "%.15g", mileage);
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
	  for (TravelerList *tl : s->clinched_by)
	    if (t == tl)
	    {	miles += s->length();
		break;
	    }
	return miles;
}

bool Route::is_valid()
{	if (!region || route.empty() || root.empty() ) return 0;
	return 1;
}

void Route::write_nmp_merged(std::string filename)
{	mkdir(filename.data(), 0777);
	filename += "/" + system->systemname;
	mkdir(filename.data(), 0777);
	filename += "/" + root + ".wpt";
	std::ofstream wptfile(filename.data());
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
