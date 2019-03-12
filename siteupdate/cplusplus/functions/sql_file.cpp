if (args.errorcheck)
	cout << et.et() << "SKIPPING database file." << endl;
else {	cout << et.et() << "Writing database file " << args.databasename << ".sql." << endl;
	// Once all data is read in and processed, create a .sql file that will
	// create all of the DB tables to be used by other parts of the project
	filename = args.databasename+".sql";
	ofstream sqlfile(filename.data());
	// Note: removed "USE" line, DB name must be specified on the mysql command line

	// we have to drop tables in the right order to avoid foreign key errors
	sqlfile << "DROP TABLE IF EXISTS datacheckErrors;\n";
	sqlfile << "DROP TABLE IF EXISTS clinchedConnectedRoutes;\n";
	sqlfile << "DROP TABLE IF EXISTS clinchedRoutes;\n";
	sqlfile << "DROP TABLE IF EXISTS clinchedOverallMileageByRegion;\n";
	sqlfile << "DROP TABLE IF EXISTS clinchedSystemMileageByRegion;\n";
	sqlfile << "DROP TABLE IF EXISTS overallMileageByRegion;\n";
	sqlfile << "DROP TABLE IF EXISTS systemMileageByRegion;\n";
	sqlfile << "DROP TABLE IF EXISTS clinched;\n";
	sqlfile << "DROP TABLE IF EXISTS segments;\n";
	sqlfile << "DROP TABLE IF EXISTS waypoints;\n";
	sqlfile << "DROP TABLE IF EXISTS connectedRouteRoots;\n";
	sqlfile << "DROP TABLE IF EXISTS connectedRoutes;\n";
	sqlfile << "DROP TABLE IF EXISTS routes;\n";
	sqlfile << "DROP TABLE IF EXISTS systems;\n";
	sqlfile << "DROP TABLE IF EXISTS updates;\n";
	sqlfile << "DROP TABLE IF EXISTS systemUpdates;\n";
	sqlfile << "DROP TABLE IF EXISTS regions;\n";
	sqlfile << "DROP TABLE IF EXISTS countries;\n";
	sqlfile << "DROP TABLE IF EXISTS continents;\n";

	// first, continents, countries, and regions
	sqlfile << "CREATE TABLE continents (code VARCHAR(3), name VARCHAR(15), PRIMARY KEY(code));\n";
	sqlfile << "INSERT INTO continents VALUES\n";
	bool first = 1;
	for (pair<string,string> c : continents)
	{	if (!first) sqlfile << ',';
		first = 0;
		sqlfile << "('" << c.first << "','" << c.second << "')\n";
	}
	sqlfile << ";\n";

	sqlfile << "CREATE TABLE countries (code VARCHAR(3), name VARCHAR(32), PRIMARY KEY(code));\n";
	sqlfile << "INSERT INTO countries VALUES\n";
	first = 1;
	for (pair<string,string> c : countries)
	{	if (!first) sqlfile << ',';
		first = 0;
		sqlfile << "('" << c.first << "','" << double_quotes(c.second) << "')\n";
	}
	sqlfile << ";\n";

	sqlfile << "CREATE TABLE regions (code VARCHAR(8), name VARCHAR(48), country VARCHAR(3), continent VARCHAR(3), regiontype VARCHAR(32), ";
	sqlfile << "PRIMARY KEY(code), FOREIGN KEY (country) REFERENCES countries(code), FOREIGN KEY (continent) REFERENCES continents(code));\n";
	sqlfile << "INSERT INTO regions VALUES\n";
	first = 1;
	for (Region &r : all_regions)
	{	if (!first) sqlfile << ',';
		first = 0;
		sqlfile << "('" << r.code << "','" + double_quotes(r.name) << "','" << r.country_code() + "','" + r.continent_code() + "','" << r.type << "')\n";
	}
	sqlfile << ";\n";

	// next, a table of the systems, consisting of the system name in the
	// field 'name', the system's country code, its full name, the default
	// color for its mapping, a level (one of active, preview, devel), and
	// a boolean indicating if the system is active for mapping in the
	// project in the field 'active'
	sqlfile << "CREATE TABLE systems (systemName VARCHAR(10), countryCode CHAR(3), fullName VARCHAR(60), color ";
	sqlfile << "VARCHAR(16), level VARCHAR(10), tier INTEGER, csvOrder INTEGER, PRIMARY KEY(systemName));\n";
	sqlfile << "INSERT INTO systems VALUES\n";
	first = 1;
	unsigned int csvOrder = 0;
	for (HighwaySystem *h : highway_systems)
	{	if (!first) sqlfile << ',';
		first = 0;
		sqlfile << "('" << h->systemname << "','" << h->country->first << "','"
			<< double_quotes(h->fullname) << "','" << h->color << "','" << h->level_name()
			<< "','" << h->tier << "','" << csvOrder << "')\n";
		csvOrder += 1;
	}
	sqlfile << ";\n";

	// next, a table of highways, with the same fields as in the first line
	sqlfile << "CREATE TABLE routes (systemName VARCHAR(10), region VARCHAR(8), route VARCHAR(16), banner VARCHAR(6), abbrev VARCHAR(3), city VARCHAR(100), root ";
	sqlfile << "VARCHAR(32), mileage FLOAT, rootOrder INTEGER, csvOrder INTEGER, PRIMARY KEY(root), FOREIGN KEY (systemName) REFERENCES systems(systemName));\n";
	sqlfile << "INSERT INTO routes VALUES\n";
	first = 1;
	csvOrder = 0;
	for (HighwaySystem *h : highway_systems)
	  for (Route &r : h->route_list)
	  {	if (!first) sqlfile << ',';
		first = 0;
		sqlfile << "(" << r.csv_line() << ",'" << csvOrder << "')\n";
		csvOrder += 1;
	  }
	sqlfile << ";\n";

	// connected routes table, but only first "root" in each in this table
	sqlfile << "CREATE TABLE connectedRoutes (systemName VARCHAR(10), route VARCHAR(16), banner VARCHAR(6), groupName VARCHAR(100), firstRoot ";
	sqlfile << "VARCHAR(32), mileage FLOAT, csvOrder INTEGER, PRIMARY KEY(firstRoot), FOREIGN KEY (firstRoot) REFERENCES routes(root));\n";
	sqlfile << "INSERT INTO connectedRoutes VALUES\n";
	first = 1;
	csvOrder = 0;
	for (HighwaySystem *h : highway_systems)
	  for (ConnectedRoute &cr : h->con_route_list)
	  {	if (!first) sqlfile << ',';
		first = 0;
		sqlfile << "(" << cr.csv_line() << ",'" << csvOrder << "')\n";
		csvOrder += 1;
	  }
	sqlfile << ";\n";

	// This table has remaining roots for any connected route
	// that connects multiple routes/roots
	sqlfile << "CREATE TABLE connectedRouteRoots (firstRoot VARCHAR(32), root VARCHAR(32), FOREIGN KEY (firstRoot) REFERENCES connectedRoutes(firstRoot));\n";
	first = 1;
	for (HighwaySystem *h : highway_systems)
	  for (ConnectedRoute &cr : h->con_route_list)
	    for (unsigned int i = 1; i < cr.roots.size(); i++)
	    {	if (first) sqlfile << "INSERT INTO connectedRouteRoots VALUES\n";
		else sqlfile << ',';
		first = 0;
		sqlfile << "('" << cr.roots[0]->root << "','" + cr.roots[i]->root + "')\n";
	    }
	sqlfile << ";\n";

	// Now, a table with raw highway route data: list of points, in order, that define the route
	sqlfile << "CREATE TABLE waypoints (pointId INTEGER, pointName VARCHAR(20), latitude DOUBLE, longitude DOUBLE, ";
	sqlfile << "root VARCHAR(32), PRIMARY KEY(pointId), FOREIGN KEY (root) REFERENCES routes(root));\n";
	unsigned int point_num = 0;
	for (HighwaySystem *h : highway_systems)
	  for (Route &r : h->route_list)
	  {	sqlfile << "INSERT INTO waypoints VALUES\n";
		first = 1;
		for (Waypoint *w : r.point_list)
		{	if (!first) sqlfile << ',';
			first = 0;
			w->point_num = point_num;
			sqlfile << "(" << w->csv_line(point_num) << ")\n";
			point_num+=1;
		}
		sqlfile << ";\n";
	  }

	// Build indices to speed latitude/longitude joins for intersecting highway queries
	sqlfile << "CREATE INDEX `latitude` ON waypoints(`latitude`);\n";
	sqlfile << "CREATE INDEX `longitude` ON waypoints(`longitude`);\n";

	// Table of all HighwaySegments.
	sqlfile << "CREATE TABLE segments (segmentId INTEGER, waypoint1 INTEGER, waypoint2 INTEGER, root VARCHAR(32), PRIMARY KEY (segmentId), FOREIGN KEY (waypoint1) ";
	sqlfile << "REFERENCES waypoints(pointId), FOREIGN KEY (waypoint2) REFERENCES waypoints(pointId), FOREIGN KEY (root) REFERENCES routes(root));\n";
	unsigned int segment_num = 0;
	vector<string> clinched_list;
	for (HighwaySystem *h : highway_systems)
	  for (Route &r : h->route_list)
	  {	sqlfile << "INSERT INTO segments VALUES\n";
		first = 1;
		for (HighwaySegment *s : r.segment_list)
		{	if (!first) sqlfile << ',';
			first = 0;
			sqlfile << '(' << s->csv_line(segment_num) << ")\n";
			for (TravelerList *t : s->clinched_by)
			  clinched_list.push_back("'" + to_string(segment_num) + "','" + t->traveler_name + "'");
			segment_num += 1;
		}
		sqlfile << ";\n";
	  }

	// maybe a separate traveler table will make sense but for now, I'll just use
	// the name from the .list name
	sqlfile << "CREATE TABLE clinched (segmentId INTEGER, traveler VARCHAR(48), FOREIGN KEY (segmentId) REFERENCES segments(segmentId));\n";
	for (size_t start = 0; start < clinched_list.size(); start += 10000)
	{	sqlfile << "INSERT INTO clinched VALUES\n";
		first = 1;
		for (size_t c = start; c < start+10000 && c < clinched_list.size(); c++)
		{	if (!first) sqlfile << ',';
			first = 0;
			sqlfile << '(' << clinched_list[c] << ")\n";
		}
		sqlfile << ";\n";
	}

	// overall mileage by region data (with concurrencies accounted for,
	// active systems only then active+preview)
	sqlfile << "CREATE TABLE overallMileageByRegion (region VARCHAR(8), activeMileage FLOAT, activePreviewMileage FLOAT);\n";
	sqlfile << "INSERT INTO overallMileageByRegion VALUES\n";
	first = 1;
	for (Region &region : all_regions)
	{	if (region.active_only_mileage+region.active_preview_mileage == 0) continue;
		if (!first) sqlfile << ',';
		first = 0;
		char fstr[65];
		sprintf(fstr, "','%.15g','%.15g')\n", region.active_only_mileage, region.active_preview_mileage);
		sqlfile << "('" << region.code << fstr;
	}
	sqlfile << ";\n";

	// system mileage by region data (with concurrencies accounted for,
	// active systems and preview systems only)
	sqlfile << "CREATE TABLE systemMileageByRegion (systemName VARCHAR(10), region VARCHAR(8), ";
	sqlfile << "mileage FLOAT, FOREIGN KEY (systemName) REFERENCES systems(systemName));\n";
	sqlfile << "INSERT INTO systemMileageByRegion VALUES\n";
	first = 1;
	for (HighwaySystem *h : highway_systems)
	  if (h->active_or_preview())
	    for (pair<Region*,double> rm : h->mileage_by_region)
	    {	if (!first) sqlfile << ',';
		first = 0;
		char fstr[35];
		sprintf(fstr, "','%.15f')\n", rm.second);
		sqlfile << "('" << h->systemname << "','" << rm.first->code << fstr;
	    }
	sqlfile << ";\n";

	// clinched overall mileage by region data (with concurrencies
	// accounted for, active systems and preview systems only)
	sqlfile << "CREATE TABLE clinchedOverallMileageByRegion (region VARCHAR(8), traveler VARCHAR(48), activeMileage FLOAT, activePreviewMileage FLOAT);\n";
	sqlfile << "INSERT INTO clinchedOverallMileageByRegion VALUES\n";
	first = 1;
	for (TravelerList *t : traveler_lists)
	  for (pair<Region*,double> rm : t->active_preview_mileage_by_region)
	  {	if (!first) sqlfile << ',';
		first = 0;
		double active_miles = 0;
		if (t->active_only_mileage_by_region.find(rm.first) != t->active_only_mileage_by_region.end())
		  active_miles = t->active_only_mileage_by_region.at(rm.first);
		char fstr[65];
		sprintf(fstr, "','%.15g','%.15g')\n", active_miles, rm.second);
		sqlfile << "('" << rm.first->code << "','" << t->traveler_name << fstr;
	  }
	sqlfile << ";\n";

	// clinched system mileage by region data (with concurrencies accounted
	// for, active systems and preview systems only)
	sqlfile << "CREATE TABLE clinchedSystemMileageByRegion (systemName VARCHAR(10), region VARCHAR(8), traveler ";
	sqlfile << "VARCHAR(48), mileage FLOAT, FOREIGN KEY (systemName) REFERENCES systems(systemName));\n";
	sqlfile << "INSERT INTO clinchedSystemMileageByRegion VALUES\n";
	first = 1;
	for (string &line : clin_db_val.csmbr_values)
	{	if (!first) sqlfile << ',';
		first = 0;
		sqlfile << line << '\n';
	}
	sqlfile << ";\n";

	// clinched mileage by connected route, active systems and preview
	// systems only
	sqlfile << "CREATE TABLE clinchedConnectedRoutes (route VARCHAR(32), traveler VARCHAR(48), mileage ";
	sqlfile << "FLOAT, clinched BOOLEAN, FOREIGN KEY (route) REFERENCES connectedRoutes(firstRoot));\n";
	for (size_t start = 0; start < clin_db_val.ccr_values.size(); start += 10000)
	{	sqlfile << "INSERT INTO clinchedConnectedRoutes VALUES\n";
		first = 1;
		for (size_t line = start; line < start+10000 && line < clin_db_val.ccr_values.size(); line++)
		{	if (!first) sqlfile << ',';
			first = 0;
			sqlfile << clin_db_val.ccr_values[line] << '\n';
		}
		sqlfile << ";\n";
	}

	// clinched mileage by route, active systems and preview systems only
	sqlfile << "CREATE TABLE clinchedRoutes (route VARCHAR(32), traveler VARCHAR(48), mileage FLOAT, clinched BOOLEAN, FOREIGN KEY (route) REFERENCES routes(root));\n";
	for (size_t start = 0; start < clin_db_val.cr_values.size(); start += 10000)
	{	sqlfile << "INSERT INTO clinchedRoutes VALUES\n";
		first = 1;
		for (size_t line = start; line < start+10000 && line < clin_db_val.cr_values.size(); line++)
		{	if (!first) sqlfile << ',';
			first = 0;
			sqlfile << clin_db_val.cr_values[line] << '\n';
		}
		sqlfile << ";\n";
	}

	// updates entries
	sqlfile << "CREATE TABLE updates (date VARCHAR(10), region VARCHAR(60), route VARCHAR(80), root VARCHAR(32), description VARCHAR(1024));\n";
	sqlfile << "INSERT INTO updates VALUES\n";
	first = 1;
	for (array<string,5> &update : updates)
	{	if (!first) sqlfile << ',';
		first = 0;
		sqlfile << "('"  << update[0] << "','" << double_quotes(update[1]) << "','" << double_quotes(update[2])
			<< "','" << update[3] << "','" << double_quotes(update[4]) << "')\n";
	}
	sqlfile << ";\n";

	// systemUpdates entries
	sqlfile << "CREATE TABLE systemUpdates (date VARCHAR(10), region VARCHAR(48), systemName VARCHAR(10), description VARCHAR(128), statusChange VARCHAR(16));\n";
	sqlfile << "INSERT INTO systemUpdates VALUES\n";
	first = 1;
	for (array<string,5> &systemupdate : systemupdates)
	{	if (!first) sqlfile << ',';
		first = 0;
		sqlfile << "('"  << systemupdate[0] << "','" << double_quotes(systemupdate[1])
			<< "','" << systemupdate[2] << "','" << double_quotes(systemupdate[3]) << "','" << systemupdate[4] << "')\n";
	}
	sqlfile << ";\n";

	// datacheck errors into the db
	sqlfile << "CREATE TABLE datacheckErrors (route VARCHAR(32), label1 VARCHAR(50), label2 VARCHAR(20), label3 VARCHAR(20), ";
	sqlfile << "code VARCHAR(20), value VARCHAR(32), falsePositive BOOLEAN, FOREIGN KEY (route) REFERENCES routes(root));\n";
	if (datacheckerrors->entries.size())
	{	sqlfile << "INSERT INTO datacheckErrors VALUES\n";
		first = 1;
		for (DatacheckEntry &d : datacheckerrors->entries)
		{	if (!first) sqlfile << ',';
			first = 0;
			sqlfile << "('" << d.route->root << "',";
			sqlfile << "'"  << d.label1 << "','" << d.label2 << "','" << d.label3 << "',";
			sqlfile << "'"  << d.code << "','" << d.info << "','" << int(d.fp) << "')\n";
		}
	}
	sqlfile << ";\n";

	// update graph info in DB if graphs were generated
	if (!args.skipgraphs)
	{	sqlfile << "DROP TABLE IF EXISTS graphs;\n";
		sqlfile << "DROP TABLE IF EXISTS graphTypes;\n";
		sqlfile << "CREATE TABLE graphTypes (category VARCHAR(12), descr VARCHAR(100), longDescr TEXT, PRIMARY KEY(category));\n";
		if (graph_types.size())
		{	sqlfile << "INSERT INTO graphTypes VALUES\n";
			first = 1;
			for (array<string,3> &g : graph_types)
			{	if (!first) sqlfile << ',';
				first = 0;
				sqlfile << "('" << g[0] << "','" << g[1] << "','" << g[2] << "')\n";
			}
			sqlfile << ";\n";
		}
	}

		sqlfile << "CREATE TABLE graphs (filename VARCHAR(32), descr VARCHAR(100), vertices INTEGER, edges INTEGER, ";
		sqlfile << "format VARCHAR(10), category VARCHAR(12), FOREIGN KEY (category) REFERENCES graphTypes(category));\n";
		if (graph_vector.size())
		{	sqlfile << "INSERT INTO graphs VALUES\n";
			for (size_t g = 0; g < graph_vector.size(); g++)
			{	if (g) sqlfile << ',';
				sqlfile << "('" << graph_vector[g].filename() << "','" << double_quotes(graph_vector[g].descr) << "','" << graph_vector[g].vertices
					<< "','" << graph_vector[g].edges << "','" << graph_vector[g].format() << "','" << graph_vector[g].category() << "')\n";
			}
			sqlfile << ";\n";
		}

	sqlfile.close();
     }
