#include "Route.h"
#include "../Args/Args.h"
#include "../ConnectedRoute/ConnectedRoute.h"
#include "../Datacheck/Datacheck.h"
#include "../DBFieldLength/DBFieldLength.h"
#include "../ErrorList/ErrorList.h"
#include "../HighwaySegment/HighwaySegment.h"
#include "../HighwaySystem/HighwaySystem.h"
#include "../Region/Region.h"
#include "../Waypoint/Waypoint.h"
#include "../../functions/tmstring.h"
#include <sys/stat.h>

std::unordered_map<std::string, Route*> Route::root_hash, Route::pri_list_hash, Route::alt_list_hash;
std::unordered_set<std::string> Route::all_wpt_files;
std::mutex Route::awf_mtx;

Route::Route(std::string &line, HighwaySystem *sys, ErrorList &el)
{	/* initialize object from a .csv file line,
	but do not yet read in waypoint file */
	con_route = 0;
	mileage = 0;
	rootOrder = -1; // order within connected route
	bools = 0;
	last_update = 0;

	// parse chopped routes csv line
	size_t NumFields = 8;
	std::string sys_str, arn_str;
	std::string* fields[8] = {&sys_str, &rg_str, &route, &banner, &abbrev, &city, &root, &arn_str};
	split(line, fields, NumFields, ';');
	if (NumFields != 8)
	{	el.add_error("Could not parse " + sys->systemname
			   + ".csv line: [" + line + "], expected 8 fields, found " + std::to_string(NumFields));
		throw 0xBAD;
	}
	if (root.empty())
	{	el.add_error("Unable to find root in " + sys->systemname +".csv line: ["+line+"]");
		throw 0xBAD;
	}
	// system
	system = sys;
	if (system->systemname != sys_str)
		el.add_error("System mismatch parsing " + system->systemname
			   + ".csv line [" + line + "], expected " + system->systemname);
	// region
	try {	region = Region::code_hash.at(rg_str);
	    }
	catch (const std::out_of_range& oor)
	    {	el.add_error("Unrecognized region in " + system->systemname
			   + ".csv line: " + line);
		region = Region::code_hash.at("error");
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
	lower(root.data());
	// alt_route_names
	upper(arn_str.data());
	size_t len;
	for (size_t pos = 0; pos < arn_str.size(); pos += len+1)
	{	len = strcspn(arn_str.data()+pos, ",");
		alt_route_names.emplace_back(arn_str, pos, len);
	}

	// insert into root_hash, checking for duplicate root entries
	if (!root_hash.insert(std::pair<std::string, Route*>(root, this)).second)
		el.add_error("Duplicate root in " + system->systemname + ".csv: " + root +
			     " already in " + root_hash.at(root)->system->systemname + ".csv");
	// insert list name into pri_list_hash, checking for duplicate .list names
	std::string list_name(readable_name());
	upper(list_name.data());
	auto it = alt_list_hash.find(list_name);
	if (it != alt_list_hash.end())
		el.add_error("Duplicate main list name in " + root + ": '" + readable_name() +
			     "' already points to " + it->second->root);
	else if (!pri_list_hash.insert(std::pair<std::string,Route*>(list_name, this)).second)
		el.add_error("Duplicate main list name in " + root + ": '" + readable_name() +
			     "' already points to " + pri_list_hash.at(list_name)->root);
	// insert alt names into alt_list_hash, checking for duplicate .list names
	for (std::string& a : alt_route_names)
	{   list_name = rg_str + ' ' + a;
	    upper(list_name.data());
	    if (pri_list_hash.count(list_name))
		el.add_error("Duplicate alt route name in " + root + ": '" + region->code + ' ' + a +
			     "' already points to " + pri_list_hash.at(list_name)->root);
	    else if (!alt_list_hash.insert(std::pair<std::string, Route*>(list_name, this)).second)
		el.add_error("Duplicate alt route name in " + root + ": '" + region->code + ' ' + a +
			     "' already points to " + alt_list_hash.at(list_name)->root);
	    // populate unused set
	    system->unusedaltroutenames.insert(list_name);
	}
}

std::string Route::str()
{	/* printable version of the object */
	return root + " (" + std::to_string(points.size) + " total points)";
}

void Route::print_route()
{	for (Waypoint& point : points)
		std::cout << point.str() << std::endl;
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

std::string Route::readable_name()
{	/* return a string for a human-readable route name */
	return rg_str + " " + route + banner + abbrev;
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

double Route::clinched_by_traveler_index(size_t t)
{	double miles = 0;
	for (HighwaySegment& s : segments)
		if (s.clinched_by[t]) miles += s.length;
	return miles;
}

/*std::string Route::list_line(int beg, int end)
{	/* Return a .list file line from (beg) to (end),
	these being indices to the points array.
	These values can be "out-of-bounds" when getting lines
	for connected routes. If so, truncate or return "". */
/*	if (beg >= int(points.size) || end <= 0) return "";
	if (end >= int(points.size)) end = points.size-1;
	if (beg < 0) beg = 0;
	return readable_name() + " " + points[beg].label + " " + points[end].label;
}//*/

void Route::write_nmp_merged()
{	std::string filename = Args::nmpmergepath + '/' + rg_str;
	mkdir(filename.data(), 0777);
	filename += "/" + system->systemname;
	mkdir(filename.data(), 0777);
	filename += "/" + root + ".wpt";
	std::ofstream wptfile(filename);
	char fstr[12];
	for (Waypoint& w : points)
	{	wptfile << w.label << ' ';
		for (std::string &a : w.alt_labels) wptfile << a << ' ';
		if (w.near_miss_points.empty())
		     {	wptfile << "http://www.openstreetmap.org/?lat=";
			sprintf(fstr, "%.6f", w.lat);
			wptfile << fstr << "&lon=";
			sprintf(fstr, "%.6f", w.lng);
			wptfile << fstr << '\n';
		     }
		else {	// for now, arbitrarily choose the northernmost
			// latitude and easternmost longitude values in the
			// list and denote a "merged" point with the https
			double lat = w.lat;
			double lng = w.lng;
			for (Waypoint *other_w : w.near_miss_points)
			{	if (other_w->lat > lat)	lat = other_w->lat;
				if (other_w->lng > lng)	lng = other_w->lng;
			}
			wptfile << "https://www.openstreetmap.org/?lat=";
			sprintf(fstr, "%.6f", lat);
			wptfile << fstr << "&lon=";
			sprintf(fstr, "%.6f", lng);
			wptfile << fstr << '\n';
			w.near_miss_points.clear();
		     }
	}
	wptfile.close();
}

size_t Route::index()
{	return this - system->routes.data;
}

Waypoint* Route::con_beg()
{	return is_reversed() ? &points.back() : points.data;
}

Waypoint* Route::con_end()
{	return is_reversed() ? points.data : &points.back();
}

void Route::set_reversed()
{	bools |= 1;
}

bool Route::is_reversed()
{	return bools & 1;
}

void Route::set_disconnected()
{	bools |= 2;
}

bool Route::is_disconnected()
{	return bools & 2;
}

// datacheck
void Route::con_mismatch()
{	if (route != con_route->route)
		Datacheck::add(this, "", "", "", "CON_ROUTE_MISMATCH",
			       route+" <-> "+con_route->route);
	if (banner != con_route->banner)
	  if (abbrev.size() && abbrev == con_route->banner)
		Datacheck::add(this, "", "", "", "ABBREV_AS_CON_BANNER", system->systemname + "," +
			       std::to_string(index()+2) + ',' + 
			       std::to_string(con_route->index()+2));
	  else	Datacheck::add(this, "", "", "", "CON_BANNER_MISMATCH",
			       (banner.size() ? banner : "(blank)") + " <-> " +
			       (con_route->banner.size() ? con_route->banner : "(blank)"));
}

void Route::mark_label_in_use(std::string& label)
{	unused_alt_labels.erase(label);
	labels_in_use.insert(move(label));
}

void Route::mark_labels_in_use(std::string& label1, std::string& label2)
{	unused_alt_labels.erase(label1);
	unused_alt_labels.erase(label2);
	labels_in_use.insert(move(label1));
	labels_in_use.insert(move(label2));
}

// sort routes by most recent update for use at end of user logs
// all should have a valid updates entry pointer before being passed here
bool sort_route_updates_oldest(const Route *r1, const Route *r2)
{	int p = strcmp(r1->last_update[0].data(), r2->last_update[0].data());
	if (!p) return r1->last_update[3] < r2->last_update[3];
		return p < 0;
}
