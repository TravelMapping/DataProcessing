#include "HighwaySystem.h"
#include "../Args/Args.h"
#include "../ConnectedRoute/ConnectedRoute.h"
#include "../DBFieldLength/DBFieldLength.h"
#include "../ErrorList/ErrorList.h"
#include "../Region/Region.h"
#include "../Route/Route.h"
#include "../TravelerList/TravelerList.h"
#include "../../functions/split.h"
#include <cstring>
#include <fstream>

std::list<HighwaySystem*> HighwaySystem::syslist;
std::list<HighwaySystem*>::iterator HighwaySystem::it;
unsigned int HighwaySystem::num_active  = 0;
unsigned int HighwaySystem::num_preview = 0;

HighwaySystem::HighwaySystem(std::string &line, ErrorList &el, std::vector<std::pair<std::string,std::string>> &countries)
{	std::ifstream file;
	// parse systems.csv line
	size_t NumFields = 6;
	std::string country_str, tier_str, level_str;
	std::string* fields[6] = {&systemname, &country_str, &fullname, &color, &tier_str, &level_str};
	split(line, fields, NumFields, ';');
	if (NumFields != 6)
	{	el.add_error("Could not parse " + Args::systemsfile
			   + " line: [" + line + "], expected 6 fields, found " + std::to_string(NumFields));
		is_valid = 0;
		return;
	}
	is_valid = 1;
	// System
	if (systemname.size() > DBFieldLength::systemName)
		el.add_error("System code > " + std::to_string(DBFieldLength::systemName)
			   + " bytes in " + Args::systemsfile + " line " + line);
	// CountryCode
	country = country_or_continent_by_code(country_str, countries);
	if (!country)
	{	el.add_error("Could not find country matching " + Args::systemsfile + " line: " + line);
		country = country_or_continent_by_code("error", countries);
	}
	// Name
	if (fullname.size() > DBFieldLength::systemFullName)
		el.add_error("System name > " + std::to_string(DBFieldLength::systemFullName)
			   + " bytes in " + Args::systemsfile + " line " + line);
	// Color
	if (color.size() > DBFieldLength::color)
		el.add_error("Color > " + std::to_string(DBFieldLength::color)
			   + " bytes in " + Args::systemsfile + " line " + line);
	// Tier
	char *endptr;
	tier = strtol(tier_str.data(), &endptr, 10);
	if (*endptr || tier < 1)
		el.add_error("Invalid tier in " + Args::systemsfile + " line " + line);
	// Level
	level = level_str[0];
	if (level_str != "active" && level_str != "preview" && level_str != "devel")
		el.add_error("Unrecognized level in " + Args::systemsfile + " line: " + line);
	switch(level)
	{	case 'a': num_active++; break;
		case 'p': num_preview++;
	}
	std::cout << systemname << '.' << std::flush;

	// read chopped routes CSV
	file.open(Args::highwaydatapath+"/hwy_data/_systems"+"/"+systemname+".csv");
	if (!file) el.add_error("Could not open "+Args::highwaydatapath+"/hwy_data/_systems"+"/"+systemname+".csv");
	else {	getline(file, line); // ignore header line
		while(getline(file, line))
		{	if (line.empty()) continue;
			// trim DOS newlines & trailing whitespace
			while ( strchr("\r\t ", line.back()) ) line.pop_back();
			Route* r = new Route(line, this, el);
				   // deleted on termination of program
			if (r->root.size()) route_list.push_back(r);
			else {	el.add_error("Unable to find root in " + systemname +".csv line: ["+line+"]");
				delete r;
			     }
		}
	     }
	file.close();

	// read connected routes CSV
	file.open(Args::highwaydatapath+"/hwy_data/_systems"+"/"+systemname+"_con.csv");
	if (!file) el.add_error("Could not open "+Args::highwaydatapath+"/hwy_data/_systems"+"/"+systemname+"_con.csv");
	else {	getline(file, line); // ignore header line
		while(getline(file, line))
		{	if (line.empty()) continue;
			// trim DOS newlines & trailing whitespace
			while ( strchr("\r\t ", line.back()) ) line.pop_back();
			con_route_list.push_back(new ConnectedRoute(line, this, el));
						 // deleted on termination of program
		}
	     }
	file.close();
}

/* Return whether this is an active system */
bool HighwaySystem::active()
{	return level == 'a';
}

/* Return whether this is a preview system */
inline bool HighwaySystem::preview()
{	return level == 'p';
}

/* Return whether this is an active or preview system */
bool HighwaySystem::active_or_preview()
{	return level == 'a' || level == 'p';
}

/* Return whether this is a development system */
bool HighwaySystem::devel()
{	return level == 'd';
}

/* Return total system mileage across all regions */
double HighwaySystem::total_mileage()
{	double mi = 0;
	for (std::pair<Region* const, double>& rm : mileage_by_region) mi += rm.second;
	return mi;
}

/* return full "active" / "preview" / "devel" string" */
std::string HighwaySystem::level_name()
{	switch(level)
	{	case 'a': return "active";
		case 'p': return "preview";
		case 'd': return "devel";
		default:  return "ERROR";
	}
}

/* Return index of a specified Route within route_list */
size_t HighwaySystem::route_index(Route* r)
{	for (size_t i = 0; i < route_list.size(); i++)
	  if (route_list[i] == r) return i;
	return -1; // error, Route not found
}

/* Return index of a specified ConnectedRoute within con_route_list */
size_t HighwaySystem::con_route_index(ConnectedRoute* cr)
{	for (size_t i = 0; i < con_route_list.size(); i++)
	  if (con_route_list[i] == cr) return i;
	return -1; // error, ConnectedRoute not found
}

void HighwaySystem::add_vertex(HGVertex* v)
{	lniu_mtx.lock();	// re-use an existing mutex...
	vertices.push_back(v);
	lniu_mtx.unlock();	// ...rather than make a new one
}

void HighwaySystem::stats_csv()
{	if (!active_or_preview()) return;
	std::ofstream sysfile(Args::csvstatfilepath + "/" + systemname + "-all.csv");
	sysfile << "Traveler,Total";
	std::list<Region*> regions;
	double total_mi = 0;
	char fstr[112];
	for (std::pair<Region* const, double>& rm : mileage_by_region)
	{	regions.push_back(rm.first);
		total_mi += rm.second;
	}
	regions.sort(sort_regions_by_code);
	for (Region *region : regions)
		sysfile << ',' << region->code;
	sysfile << '\n';
	for (TravelerList *t : TravelerList::allusers)
	{	// only include entries for travelers who have any mileage in system
		auto it = t->system_region_mileages.find(this);
		if (it != t->system_region_mileages.end())
		{	sprintf(fstr, ",%.2f", t->system_region_miles(this));
			sysfile << t->traveler_name << fstr;
			for (Region *region : regions)
			  try {	sprintf(fstr, ",%.2f", it->second.at(region));
				sysfile << fstr;
			      }
			  catch (const std::out_of_range& oor)
			      {	sysfile << ",0";
			      }
			sysfile << '\n';
		}
	}
	sprintf(fstr, "TOTAL,%.2f", total_mileage());
	sysfile << fstr;
	for (Region *region : regions)
	{	sprintf(fstr, ",%.2f", mileage_by_region.at(region));
		sysfile << fstr;
	}
	sysfile << '\n';
	sysfile.close();
}
