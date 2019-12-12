//FIXME try to break strtok. That goes for all strtok project-wide.
class HighwaySystem
{	/* This class encapsulates the contents of one .csv file
	that represents the collection of highways within a system.

	See Route for information about the fields of a .csv file

	With the implementation of three tiers of systems (active,
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
	unsigned short tier;
	char level; // 'a' for active, 'p' for preview, 'd' for devel

	std::list<Route> route_list;
	std::list<ConnectedRoute> con_route_list;
	std::unordered_map<Region*, double> mileage_by_region;
	std::unordered_set<HGVertex*> vertices;

	HighwaySystem(std::string &line, ErrorList &el, std::string path, std::string &systemsfile,
		      std::list<std::pair<std::string,std::string>> &countries,
		      std::unordered_map<std::string, Region*> &region_hash)
	{	char *c_country = 0;
		std::ifstream file;

		// parse systems.csv line
		if (line.back() == 0x0D) line.erase(line.end()-1);	// trim DOS newlines
		char *c_line = new char[line.size()+1];
		strcpy(c_line, line.data());
		systemname = strtok(c_line, ";");
		c_country = strtok(0, ";");
		fullname = strtok(0, ";");
		color = strtok(0, ";");
		tier = *strtok(0, ";")-48;
		level = *strtok(0, ";");
		if (tier > 5 || tier < 1 || color.empty() || fullname.empty() || !c_country || systemname.empty() || invalid_level())
		{	el.add_error("Could not parse "+systemsfile+" line: "+line);
			delete[] c_line;
			return;
		}
		country = country_or_continent_by_code(c_country, countries);
		if (!country) el.add_error("Could not find country matching " + systemsfile + " line: " + line);
		delete[] c_line;

		// read chopped routes CSV
		file.open(path+"/"+systemname+".csv");
		if (!file) el.add_error("Could not open "+path+"/"+systemname+".csv");
		else {	getline(file, line); // ignore header line
			while(getline(file, line))
			{	if (line.back() == 0x0D) line.erase(line.end()-1);	// trim DOS newlines
				route_list.emplace_back(line, this, el, region_hash);
				if (!route_list.back().is_valid()) route_list.pop_back();
			}
		     }
		file.close();

		// read connected routes CSV
		file.open(path+"/"+systemname+"_con.csv");
		if (!file) el.add_error("Could not open "+path+"/"+systemname+"_con.csv");
		else {	getline(file, line); // ignore header line
			while(getline(file, line))
			{	if (line.back() == 0x0D) line.erase(line.end()-1);	// trim DOS newlines
				con_route_list.emplace_back(line, this, el, route_list);
			}
		     }
		file.close();
	}

	bool is_valid()
	{	if (invalid_level()) return 0;
		if (tier > 5 || tier < 1 || color.empty() || fullname.empty() || !country || systemname.empty()) return 0;
		return 1;
	}

	bool invalid_level()
	{	return level != 'a' && level != 'p' && level != 'd';
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
		for (std::pair<Region*, double> rm : mileage_by_region) mi += rm.second;
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
