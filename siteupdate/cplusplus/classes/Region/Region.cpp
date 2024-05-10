#include "Region.h"
#include "../DBFieldLength/DBFieldLength.h"
#include "../ErrorList/ErrorList.h"
#include "../GraphGeneration/HGEdge.h"
#include "../GraphGeneration/HGVertex.h"
#include "../HighwaySegment/HighwaySegment.h"
#include "../HighwaySystem/HighwaySystem.h"
#include "../Route/Route.h"
#include "../Waypoint/Waypoint.h"
#include "../../functions/tmstring.h"

std::pair<std::string, std::string> *country_or_continent_by_code(std::string code, std::vector<std::pair<std::string, std::string>> &pair_vector)
{	for (std::pair<std::string, std::string>& c : pair_vector)
		if (c.first == code) return &c;
	return 0;
}

TMArray<Region> Region::allregions;
Region* Region::it;
std::unordered_map<std::string, Region*> Region::code_hash;
std::vector<std::pair<std::string, std::string>> Region::continents;
std::vector<std::pair<std::string, std::string>> Region::countries;

Region::Region (const std::string &line, ErrorList &el)
{	active_only_mileage = 0;
	active_preview_mileage = 0;
	overall_mileage = 0;
	// parse CSV line
	size_t NumFields = 5;
	std::string country_str, continent_str;
	std::string* fields[5] = {&code, &name, &country_str, &continent_str, &type};
	split(line, fields, NumFields, ';');
	if (NumFields != 5)
	{	el.add_error("Could not parse regions.csv line: [" + line + "], expected 5 fields, found " + std::to_string(NumFields));
		continent = country_or_continent_by_code("error", continents);
		country   = country_or_continent_by_code("error", countries);
		throw 0xBAD;
	}
	// code
	if (code.size() > DBFieldLength::regionCode)
		el.add_error("Region code > " + std::to_string(DBFieldLength::regionCode)
			   + " bytes in regions.csv line " + line);
	// name
	if (name.size() > DBFieldLength::regionName)
		el.add_error("Region name > " + std::to_string(DBFieldLength::regionName)
			   + " bytes in regions.csv line " + line);
	// country
	country = country_or_continent_by_code(country_str, countries);
	if (!country)
	{	el.add_error("Could not find country matching regions.csv line: " + line);
		country = country_or_continent_by_code("error", countries);
	}
	// continent
	continent = country_or_continent_by_code(continent_str, continents);
	if (!continent)
	{	el.add_error("Could not find continent matching regions.csv line: " + line);
		continent = country_or_continent_by_code("error", continents);
	}
	// regionType
	if (type.size() > DBFieldLength::regiontype)
		el.add_error("Region type > " + std::to_string(DBFieldLength::regiontype)
			   + " bytes in regions.csv line " + line);
}

std::string& Region::country_code()
{	return country->first;
}

std::string& Region::continent_code()
{	return continent->first;
}

void Region::ve_thread(std::mutex* mtx, std::vector<HGVertex>* vertices, TMArray<HGEdge>* edges)
{	while (it != allregions.end())
	{	for (mtx->lock(); it != allregions.end(); it++)
		  if (it->active_preview_mileage) break;
		if (it == allregions.end()) return mtx->unlock();
		Region& rg = *it++;
		mtx->unlock();

		rg.vertices.alloc(vertices->data(), vertices->size());
		rg.edges.alloc(edges->data, edges->size);
		for (Route* r : rg.routes)
		  if (r->system->active_or_preview())
		    for (Waypoint& w : r->points)
		    { HGVertex* v = w.hashpoint()->vertex;
		      if (rg.vertices.add_value(v))
			for (HGEdge* e : v->incident_edges)
			  if (e->segment->route->region == &rg)
			    rg.edges.add_value(e);
		    }
		rg.vertices.shrink_to_fit();
		rg.edges.shrink_to_fit();
	}
}
