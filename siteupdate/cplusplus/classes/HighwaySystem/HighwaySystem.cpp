#define FMT_HEADER_ONLY
#include "HighwaySystem.h"
#include "../Args/Args.h"
#include "../ConnectedRoute/ConnectedRoute.h"
#include "../DBFieldLength/DBFieldLength.h"
#include "../ErrorList/ErrorList.h"
#include "../GraphGeneration/HGEdge.h"
#include "../GraphGeneration/HGVertex.h"
#include "../HighwaySegment/HighwaySegment.h"
#include "../Region/Region.h"
#include "../Route/Route.h"
#include "../TravelerList/TravelerList.h"
#include "../Waypoint/Waypoint.h"
#include "../../functions/tmstring.h"
#include <fmt/format.h>
#include <fstream>

TMArray<HighwaySystem> HighwaySystem::syslist;
HighwaySystem* HighwaySystem::it;
std::unordered_map<std::string, HighwaySystem*> HighwaySystem::sysname_hash;
unsigned int HighwaySystem::num_active  = 0;
unsigned int HighwaySystem::num_preview = 0;

HighwaySystem::HighwaySystem(std::string &line, ErrorList &el)
{	std::ifstream file;
	// parse systems.csv line
	size_t NumFields = 6;
	std::string country_str, tier_str, level_str;
	std::string* fields[6] = {&systemname, &country_str, &fullname, &color, &tier_str, &level_str};
	split(line, fields, NumFields, ';');
	if (NumFields != 6)
	{	el.add_error("Could not parse " + Args::systemsfile
			   + " line: [" + line + "], expected 6 fields, found " + std::to_string(NumFields));
		throw 0xf1e1d5;
	}
	// System
	if (systemname.size() > DBFieldLength::systemName)
		el.add_error("System code > " + std::to_string(DBFieldLength::systemName)
			   + " bytes in " + Args::systemsfile + " line " + line);
	if (!sysname_hash.emplace(systemname, this).second)
	{	el.add_error("Duplicate system code " + systemname
			   + " in " + Args::systemsfile + " line " + line);
		throw 0xc0de;
	}
	// CountryCode
	country = country_or_continent_by_code(country_str, Region::countries);
	if (!country)
	{	el.add_error("Could not find country matching " + Args::systemsfile + " line: " + line);
		country = country_or_continent_by_code("error", Region::countries);
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
	is_subgraph_system = 0;
	std::cout /*<< systemname*/ << '.' << std::flush;

	// read chopped routes CSV
	file.open(Args::datapath+"/data/_systems/"+systemname+".csv");
	if (!file) el.add_error("Could not open "+Args::datapath+"/data/_systems/"+systemname+".csv");
	else {	getline(file, line); // ignore header line
		std::vector<std::string> lines;
		while(getline(file, line))
		{	// trim DOS newlines & trailing whitespace
			while ( line.size() && strchr("\r\t ", line.back()) ) line.pop_back();
			if (line.size()) lines.emplace_back(std::move(line));
		}
		Route* r = routes.alloc(lines.size());
		for (std::string& l : lines)
		  try {	new(r) Route(l, this, el);
			// placement new
			r->region->routes.push_back(r);
			r++;
		      }
		  catch (const int) {--routes.size;}
	     }
	file.close();

	// read connected routes CSV
	file.open(Args::datapath+"/data/_systems/"+systemname+"_con.csv");
	if (!file) el.add_error("Could not open "+Args::datapath+"/data/_systems/"+systemname+"_con.csv");
	else {	getline(file, line); // ignore header line
		std::vector<std::string> lines;
		while(getline(file, line))
		{	// trim DOS newlines & trailing whitespace
			while ( line.size() && strchr("\r\t ", line.back()) ) line.pop_back();
			if (line.size()) lines.emplace_back(std::move(line));
		}
		ConnectedRoute* cr = con_routes.alloc(lines.size());
		for (std::string& l : lines)
		  try {	new(cr) ConnectedRoute(l, this, el);
			// placement new
			cr++;
		      }
		  catch (const int) {--con_routes.size;}
	     }
	file.close();
}

void HighwaySystem::systems_csv(ErrorList& el)
{	std::ifstream file;
	std::string line;
	file.open(Args::datapath+"/"+Args::systemsfile);
	if (!file) el.add_error("Could not open "+Args::datapath+"/"+Args::systemsfile);
	else {	getline(file, line); // ignore header line
		std::vector<std::string> lines;
		while(getline(file, line)) if (line.size())
		{	if (line.back() == 0x0D) line.pop_back();	// trim DOS newlines
			if (line[0] == '#') continue;
			if (strchr(line.data(), '"'))
				el.add_error("Double quotes in systems.csv line: "+line);
			lines.emplace_back(std::move(line));
		}
		it = syslist.alloc(lines.size());
		for (std::string& l : lines)
		  try {	new(it) HighwaySystem(l, el);
			// placement new
			++it;
		      }
		  catch (const int) {--syslist.size;}
		std::cout << '!' << std::endl;
	     }
	file.close();
}

void HighwaySystem::ve_thread(std::mutex* mtx, std::vector<HGVertex>* vertices, TMArray<HGEdge>* edges)
{	while (it != syslist.end())
	{	for (mtx->lock(); it != syslist.end(); it++)
		  if (it->is_subgraph_system) break;
		if (it == syslist.end()) return mtx->unlock();
		HighwaySystem& h = *it++;
		mtx->unlock();

		h.vertices.alloc(vertices->data(), vertices->size());
		h.edges.alloc(edges->data, edges->size);
		for (Route& r : h.routes)
		  for (Waypoint& w : r.points)
		  { HGVertex* v = w.hashpoint()->vertex;
		    if (h.vertices.add_value(v))
		      for (HGEdge* e : v->incident_edges)
			if (e->segment->concurrent)
			{ for (HighwaySegment* s : *e->segment->concurrent)
			    if (s->route->system == &h)
			    {	h.edges.add_value(e);
				break;
			    }
			}
			else if (e->segment->route->system == &h)
				h.edges.add_value(e);
		  }
		h.vertices.shrink_to_fit();
		h.edges.shrink_to_fit();
	}
}

/* Return whether this is an active system */
bool HighwaySystem::active()
{	return level == 'a';
}

/* Return whether this is a preview system */
bool HighwaySystem::preview()
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

/* Return full "active" / "preview" / "devel" string */
std::string HighwaySystem::level_name()
{	switch(level)
	{	case 'a': return "active";
		case 'p': return "preview";
		case 'd': return "devel";
		default:  return "ERROR";
	}
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
	regions.sort();
	for (Region *region : regions)
		sysfile << ',' << region->code;
	sysfile << '\n';
	for (TravelerList& t : TravelerList::allusers)
	{	// only include entries for travelers who have any mileage in system
		auto srm_it = t.system_region_mileages.find(this);
		if (srm_it != t.system_region_mileages.end())
		{	*fmt::format_to(fstr, ",{:.2f}", t.system_miles(this)) = 0;
			sysfile << t.traveler_name << fstr;
			for (Region *region : regions)
			{	auto trm_it = srm_it->second.find(region);
				if (trm_it != srm_it->second.end())
				{	*fmt::format_to(fstr, ",{:.2f}", trm_it->second) = 0;
					sysfile << fstr;
				}
				else	sysfile << ",0";
			}
			sysfile << '\n';
		}
	}
	*fmt::format_to(fstr, "TOTAL,{:.2f}", total_mileage()) = 0;
	sysfile << fstr;
	for (Region *region : regions)
	{	*fmt::format_to(fstr, ",{:.2f}", mileage_by_region.at(region)) = 0;
		sysfile << fstr;
	}
	sysfile << '\n';
	sysfile.close();
}

void HighwaySystem::mark_route_in_use(std::string& lookup)
{	mtx.lock();
	unusedaltroutenames.erase(lookup);
	listnamesinuse.insert(std::move(lookup));
	mtx.unlock();
}

void HighwaySystem::mark_routes_in_use(std::string& lookup1, std::string& lookup2)
{	mtx.lock();
	unusedaltroutenames.erase(lookup1);
	unusedaltroutenames.erase(lookup2);
	listnamesinuse.insert(std::move(lookup1));
	listnamesinuse.insert(std::move(lookup2));
	mtx.unlock();
}
