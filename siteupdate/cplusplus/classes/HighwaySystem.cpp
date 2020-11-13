class HighwaySystem
{	/* This class encapsulates the contents of one .csv file
	that represents the collection of highways within a system.

	See Route for information about the fields of a .csv file

	With the implementation of three levels of systems (active,
	preview, devel), added parameter and field here, to be stored in
	DB

	After construction and when all Route entries are made, a _con.csv
	file is read that defines the connected routes in the system.
	In most cases, the connected route is just a single Route, but when
	a designation within the same system crosses region boundaries,
	a connected route defines the entirety of the route.
	*/

	public:
	std::string systemname;
	std::pair<std::string, std::string> *country;
	std::string fullname;
	std::string color;
	short tier;
	char level; // 'a' for active, 'p' for preview, 'd' for devel

	std::list<Route> route_list;
	std::list<ConnectedRoute> con_route_list;
	std::unordered_map<Region*, double> mileage_by_region;
	std::unordered_set<HGVertex*> vertices;
	std::unordered_set<std::string>listnamesinuse, unusedaltroutenames;
	std::mutex lniu_mtx, uarn_mtx;
	bool is_valid;

	HighwaySystem(std::string &line, ErrorList &el, std::string path, std::string &systemsfile,
		      std::vector<std::pair<std::string,std::string>> &countries,
		      std::unordered_map<std::string, Region*> &region_hash)
	{	std::ifstream file;
		// parse systems.csv line
		size_t NumFields = 6;
		std::string country_str, tier_str, level_str;
		std::string* fields[6] = {&systemname, &country_str, &fullname, &color, &tier_str, &level_str};
		split(line, fields, NumFields, ';');
		if (NumFields != 6)
		{	el.add_error("Could not parse " + systemsfile
				   + " line: [" + line + "], expected 6 fields, found " + std::to_string(NumFields));
			is_valid = 0;
			return;
		}
		is_valid = 1;
		// System
		if (systemname.size() > DBFieldLength::systemName)
			el.add_error("System code > " + std::to_string(DBFieldLength::systemName)
				   + " bytes in " + systemsfile + " line " + line);
		// CountryCode
		country = country_or_continent_by_code(country_str, countries);
		if (!country)
		{	el.add_error("Could not find country matching " + systemsfile + " line: " + line);
			country = country_or_continent_by_code("error", countries);
		}
		// Name
		if (fullname.size() > DBFieldLength::systemFullName)
			el.add_error("System name > " + std::to_string(DBFieldLength::systemFullName)
				   + " bytes in " + systemsfile + " line " + line);
		// Color
		if (color.size() > DBFieldLength::color)
			el.add_error("Color > " + std::to_string(DBFieldLength::color)
				   + " bytes in " + systemsfile + " line " + line);
		// Tier
		char *endptr;
		tier = strtol(tier_str.data(), &endptr, 10);
		if (*endptr || tier < 1)
			el.add_error("Invalid tier in " + systemsfile + " line " + line);
		// Level
		level = level_str[0];
		if (level_str != "active" && level_str != "preview" && level_str != "devel")
			el.add_error("Unrecognized level in " + systemsfile + " line: " + line);

		std::cout << systemname << '.' << std::flush;

		// read chopped routes CSV
		file.open(path+"/"+systemname+".csv");
		if (!file) el.add_error("Could not open "+path+"/"+systemname+".csv");
		else {	getline(file, line); // ignore header line
			while(getline(file, line))
			{	// trim DOS newlines & trailing whitespace
				while ( strchr("\r\t ", line.back()) ) line.pop_back();
				if (line.empty()) continue;
				route_list.emplace_back(line, this, el, region_hash);
				if (route_list.back().root.empty())
				{	el.add_error("Unable to find root in " + systemname +".csv line: ["+line+"]");
					route_list.pop_back();
				}
			}
		     }
		file.close();

		// read connected routes CSV
		file.open(path+"/"+systemname+"_con.csv");
		if (!file) el.add_error("Could not open "+path+"/"+systemname+"_con.csv");
		else {	getline(file, line); // ignore header line
			while(getline(file, line))
			{	// trim DOS newlines & trailing whitespace
				while ( strchr("\r\t ", line.back()) ) line.pop_back();
				if (line.empty()) continue;
				con_route_list.emplace_back(line, this, el);
			}
		     }
		file.close();
	}

	/* Return whether this is an active system */
	bool active()
	{	return level == 'a';
	}

	/* Return whether this is a preview system */
	bool preview()
	{	return level == 'p';
	}

	/* Return whether this is an active or preview system */
	bool active_or_preview()
	{	return level == 'a' || level == 'p';
	}

	/* Return whether this is a development system */
	bool devel()
	{	return level == 'd';
	}

	/* Return total system mileage across all regions */
	double total_mileage()
	{	double mi = 0;
		for (std::pair<Region* const, double>& rm : mileage_by_region) mi += rm.second;
		return mi;
	}

	std::string level_name()
	{	switch(level)
		{	case 'a': return "active";
			case 'p': return "preview";
			case 'd': return "devel";
			default:  return "ERROR";
		}
	}
};
