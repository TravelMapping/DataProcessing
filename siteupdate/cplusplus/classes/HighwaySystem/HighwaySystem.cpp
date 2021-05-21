#include "HighwaySystem.h"
#include "../Args/Args.h"
#include "../ConnectedRoute/ConnectedRoute.h"
#include "../DBFieldLength/DBFieldLength.h"
#include "../ErrorList/ErrorList.h"
#include "../Region/Region.h"
#include "../Route/Route.h"
#include "../../functions/split.h"
#include <cstring>
#include <fstream>

std::list<HighwaySystem*> HighwaySystem::syslist;
std::list<HighwaySystem*>::iterator HighwaySystem::it;

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

void HighwaySystem::insert_vertex(HGVertex* v)
{	lniu_mtx.lock();	// re-use an existing mutex...
	vertices.insert(v);
	lniu_mtx.unlock();	// ...rather than make a new one
}
