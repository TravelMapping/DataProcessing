#include "Region.h"
#include "../DBFieldLength/DBFieldLength.h"
#include "../ErrorList/ErrorList.h"
#include "../../functions/split.h"

std::pair<std::string, std::string> *country_or_continent_by_code(std::string code, std::vector<std::pair<std::string, std::string>> &pair_vector)
{	for (std::pair<std::string, std::string>& c : pair_vector)
		if (c.first == code) return &c;
	return 0;
}

std::vector<Region*> Region::allregions;
std::unordered_map<std::string, Region*> Region::code_hash;

Region::Region (const std::string &line,
		std::vector<std::pair<std::string, std::string>> &countries,
		std::vector<std::pair<std::string, std::string>> &continents,
		ErrorList &el)
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
		is_valid = 0;
		return;
	}
	is_valid = 1;
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

void Region::insert_vertex(HGVertex* v)
{	mtx.lock();
	vertices.insert(v);
	mtx.unlock();
}

bool sort_regions_by_code(const Region *r1, const Region *r2)
{	return r1->code < r2->code;
}
