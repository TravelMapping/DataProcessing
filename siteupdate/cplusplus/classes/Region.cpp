std::pair<std::string, std::string> *country_or_continent_by_code(std::string code, std::list<std::pair<std::string, std::string>> &pair_list)
{	for (std::list<std::pair<std::string, std::string>>::iterator c = pair_list.begin(); c != pair_list.end(); c++)
		if (c->first == code) return &*c;
	return 0;
}

class Region
{   /* This class encapsulates the contents of one .csv file line
    representing a region in regions.csv

    The format of the .csv file for a region is a set of
    semicolon-separated lines, each of which has 5 fields:

    ﻿code;name;country;continent;regionType

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

	Region (std::string &line,
		std::list<std::pair<std::string, std::string>> &countries,
		std::list<std::pair<std::string, std::string>> &continents,
		ErrorList &el)
	{	active_only_mileage = 0;
		active_preview_mileage = 0;
		overall_mileage = 0;
		ao_mi_mtx = new std::mutex;
		ap_mi_mtx = new std::mutex;
		ov_mi_mtx = new std::mutex;
			    // deleted on termination of program
		char *c_country = 0;
		char *c_continent = 0;
		// parse CSV line
		if (line.back() == 0x0D) line.erase(line.end()-1);	// trim DOS newlines
		char *c_line = new char[line.size()+1];
		strcpy(c_line, line.data());
		code = strtok(c_line, ";");
		name = strtok(0, ";");
		c_country = strtok(0, ";");
		c_continent = strtok(0, ";");
		type = strtok(0, ";");
		if (code.empty() || name.empty() || !c_country || !c_continent || type.empty())
		{	el.add_error("Could not parse regions.csv line: " + line);
			delete[] c_line;
			return;
		}

		country = country_or_continent_by_code(c_country, countries);
		if (!country) el.add_error("Could not find country matching regions.csv line: " + line);

		continent = country_or_continent_by_code(c_continent, continents);
		if (!continent) el.add_error("Could not find continent matching regions.csv line: " + line);

		delete[] c_line;
	}

	std::string &country_code()
	{	return country->first;
	}

	std::string &continent_code()
	{	return continent->first;
	}

	bool is_valid()
	{	return country && continent;
	}
};

bool sort_regions_by_code(const Region *r1, const Region *r2)
{	return r1->code < r2->code;
}
