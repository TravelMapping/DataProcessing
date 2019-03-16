class ConnectedRoute
{   /* This class encapsulates a single 'connected route' as given
    by a single line of a _con.csv file
    */

	public:
	HighwaySystem *system;
	std::string route;
	std::string banner;
	std::string groupname;
	std::vector<Route*> roots;

	double mileage; // will be computed for routes in active & preview systems

	ConnectedRoute(std::string &line, HighwaySystem *sys, ErrorList &el, std::list<Route> &route_list)
	{	mileage = 0;

		/* initialize the object from the _con.csv line given */
		if (line.back() == 0x0D) line.erase(line.end()-1);	// trim DOS newlines

		// system
		size_t left = line.find(';');
		if (left == std::string::npos)
		{	el.add_error("Could not parse " + system->systemname + "_con.csv line: [" + line + "], expected 5 fields, found 1");
			return;
		}
		system = sys;
		if (system->systemname != line.substr(0,left))
		    {	el.add_error("System mismatch parsing " + system->systemname + "_con.csv line [" + line + "], expected " + system->systemname);
			return;
		    }

		// route
		size_t right = line.find(';', left+1);
		if (right == std::string::npos)
		{	el.add_error("Could not parse " + system->systemname + "_con.csv line: [" + line + "], expected 5 fields, found 2");
			return;
		}
		route = line.substr(left+1, right-left-1);

		// banner
		left = right;
		right = line.find(';', left+1);
		if (right == std::string::npos)
		{	el.add_error("Could not parse " + system->systemname + "_con.csv line: [" + line + "], expected 5 fields, found 3");
			return;
		}
		banner = line.substr(left+1, right-left-1);
		if (banner.size() > 6)
		    {	el.add_error("Banner >6 characters in " + system->systemname + "_con.csv line: [" + line + "], " + banner);
			return;
		    }

		// groupname
		left = right;
		right = line.find(';', left+1);
		if (right == std::string::npos)
		{	el.add_error("Could not parse " + system->systemname + "_con.csv line: [" + line + "], expected 5 fields, found 4");
			return;
		}
		groupname = line.substr(left+1, right-left-1);

		// roots
		left = right;
		right = line.find(';', left+1);
		if (right != std::string::npos)
		{	el.add_error("Could not parse " + system->systemname + "_con.csv line: [" + line + "], expected 5 fields, found more");
			return;
		}
		char *rootstr = new char[line.size()-left];
		strcpy(rootstr, line.substr(left+1).data());
		int rootOrder = 0;
		for (char *token = strtok(rootstr, ","); token; token = strtok(0, ","))
		{	Route *root = route_by_root(token, route_list);
			if (!root) el.add_error("Could not find Route matching root " + std::string(token) + " in system " + system->systemname + '.');
			else {	roots.push_back(root);
				// save order of route in connected route
				root->rootOrder = rootOrder;
			     }
			rootOrder++;
		}
		delete[] rootstr;
		if (roots.size() < 1) el.add_error("No roots in " + system->systemname + "_con.csv line [" + line + "]");
	}

	std::string connected_rtes_line()
	{	/* return a connected routes system csv line, for debug purposes */
		std::string line = system->systemname + ';' + route + ';' + banner + ';' + groupname + ';';
		if (!roots.empty())
		{	line += roots[0]->root;
			for (size_t i = 1; i < roots.size(); i++)
				line += ',' + roots[i]->root;
		}
		return line;
	}

	std::string csv_line()
	{	/* return csv line to insert into a table */
		char fstr[32];
		sprintf(fstr, "','%.15g'", mileage);
		return "'" + system->systemname + "','" + route + "','" + banner + "','" + double_quotes(groupname) + "','" + roots[0]->root + fstr;
	}

	std::string readable_name()
	{	/* return a string for a human-readable connected route name */
		std::string ans = route + banner;
		if (!groupname.empty()) ans += " (" +  groupname + ")";
		return ans;
	}
};
