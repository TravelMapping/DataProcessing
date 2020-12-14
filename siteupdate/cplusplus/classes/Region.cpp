#include "../classes/DBFieldLength/DBFieldLength.h"
#include "../functions/split.h"

std::pair<std::string, std::string> *country_or_continent_by_code(std::string code, std::vector<std::pair<std::string, std::string>> &pair_vector)
{	for (std::pair<std::string, std::string>& c : pair_vector)
		if (c.first == code) return &c;
	return 0;
}

class Region
{   /* This class encapsulates the contents of one .csv file line
    representing a region in regions.csv

    The format of the .csv file for a region is a set of
    semicolon-separated lines, each of which has 5 fields:

    code;name;country;continent;regionType

    The first line names these fields, subsequent lines exist,
    one per region, with values for each field.

    code: the region code, as used in .list files, or HB or stats
    queries, etc.
    E.G. AB, ABW, ACT, etc.

    name: the full name of the region
    E.G. Alberta, Aruba, Australian Capital Territory, etc.

    country: the code for the country in which the region is located
    E.G. CAN, NLD, AUS, etc.

    continent: the code for the continent on which the region is located
    E.G. NA, OCE, ASI, etc.

    regionType: a description of the region type
    E.G. Province, Constituent Country, Territory, etc.
    */

	public:
	std::pair<std::string, std::string> *country, *continent;
	std::string code, name, type;
	double active_only_mileage;
	double active_preview_mileage;
	double overall_mileage;
	std::mutex *ao_mi_mtx, *ap_mi_mtx, *ov_mi_mtx;
	std::unordered_set<HGVertex*> vertices;
	bool is_valid;

	Region (const std::string &line,
		std::vector<std::pair<std::string, std::string>> &countries,
		std::vector<std::pair<std::string, std::string>> &continents,
		ErrorList &el)
	{	active_only_mileage = 0;
		active_preview_mileage = 0;
		overall_mileage = 0;
		ao_mi_mtx = new std::mutex;
		ap_mi_mtx = new std::mutex;
		ov_mi_mtx = new std::mutex;
			    // deleted on termination of program
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

	std::string &country_code()
	{	return country->first;
	}

	std::string &continent_code()
	{	return continent->first;
	}
};

bool sort_regions_by_code(const Region *r1, const Region *r2)
{	return r1->code < r2->code;
}
