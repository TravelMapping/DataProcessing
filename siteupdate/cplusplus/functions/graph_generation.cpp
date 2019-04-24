// Build a graph structure out of all highway data in active and
// preview systems
cout << et.et() << "Setting up for graphs of highway data." << endl;
HighwayGraph graph_data(all_waypoints, highway_systems, datacheckerrors, args.numthreads, et);

cout << et.et() << "Writing graph waypoint simplification log." << endl;
filename = args.logfilepath + "/waypointsimplification.log";
ofstream wslogfile(filename.data());
for (string line : graph_data.waypoint_naming_log)
	wslogfile << line << '\n';
wslogfile.close();
graph_data.waypoint_naming_log.clear();

// create list of graph information for the DB
vector<GraphListEntry> graph_vector;
list<array<string,3>> graph_types;

// start generating graphs and making entries for graph DB table

if (args.skipgraphs || args.errorcheck)
	cout << et.et() << "SKIPPING generation of subgraphs." << endl;
else {	list<Region*> *regions;
	list<HighwaySystem*> *systems;

	graph_vector.emplace_back("tm-master", "All Travel Mapping Data", 's', 'M', (list<Region*>*)0, (list<HighwaySystem*>*)0, (PlaceRadius*)0);
	graph_vector.emplace_back("tm-master", "All Travel Mapping Data", 'c', 'M', (list<Region*>*)0, (list<HighwaySystem*>*)0, (PlaceRadius*)0);
	graph_vector.emplace_back("tm-master", "All Travel Mapping Data", 't', 'M', (list<Region*>*)0, (list<HighwaySystem*>*)0, (PlaceRadius*)0);

      /*#ifdef threading_enabled
	if (args.numthreads <= 1)
      #endif//*/
	     {	cout << et.et() << "Writing master TM simple graph file, tm-master-simple.tmg" << endl;
		graph_data.write_master_tmg_simple(&graph_vector[0], args.graphfilepath+"/tm-master-simple.tmg");
		cout << et.et() << "Writing master TM collapsed graph file, tm-master-collapsed.tmg." << endl;
		graph_data.write_master_tmg_collapsed(&graph_vector[1], args.graphfilepath+"/tm-master-collapsed.tmg", 0);
		cout << et.et() << "Writing master TM traveled graph file, tm-master.tmg." << endl;
		graph_data.write_master_tmg_traveled(&graph_vector[2], args.graphfilepath+"/tm-master-traveled.tmg", &traveler_lists, 0);
	     }
      /*#ifdef threading_enabled
	else {	cout << et.et() << "Writing master TM simple graph file, tm-master-simple.tmg" << endl;
		thr[0] = new thread(MasterTmgSimpleThread, &graph_data, &graph_vector[0], args.graphfilepath+"/tm-master-simple.tmg");
		cout << et.et() << "Writing master TM collapsed graph file, tm-master-collapsed.tmg." << endl;
		thr[1] = new thread(MasterTmgCollapsedThread, &graph_data, &graph_vector[1], args.graphfilepath+"/tm-master-collapsed.tmg");
		thr[0]->join();
		thr[1]->join();
		delete thr[0];
		delete thr[1];
	     }
      #endif//*/

	graph_types.push_back({"master", "All Travel Mapping Data", "These graphs contain all routes currently plotted in the Travel Mapping project."});
	size_t graphnum = 3;


	// graphs restricted by place/area - from areagraphs.csv file
	cout << '\n' << et.et() << "Creating area data graphs." << endl;
	filename = args.highwaydatapath+"/graphs/areagraphs.csv";
	file.open(filename.data());
	getline(file, line);  // ignore header line
	list<PlaceRadius> area_list;
	while (getline(file, line))
	{	vector<char*> fields;
		char *cline = new char[line.size()+1];
		strcpy(cline, line.data());
		for (char *token = strtok(cline, ";"); token; token = strtok(0, ";")) fields.push_back(token);
		if (fields.size() != 5)
		{	cout << "Could not parse areagraphs.csv line: " << cline << endl;
			delete[] cline;
			continue;
		}
		area_list.emplace_back(fields[0], fields[1], fields[2], fields[3], fields[4]);
		delete[] cline;
	}
	file.close();

	// add entries to graph_vector
	for (PlaceRadius &a : area_list)
	{	graph_vector.emplace_back(a.base + to_string(a.r) + "-area", a.place + " (" + to_string(a.r) + " mi radius)",
					  's', 'a', (list<Region*>*)0, (list<HighwaySystem*>*)0, &a);
		graph_vector.emplace_back(a.base + to_string(a.r) + "-area", a.place + " (" + to_string(a.r) + " mi radius)",
					  'c', 'a', (list<Region*>*)0, (list<HighwaySystem*>*)0, &a);
		graph_vector.emplace_back(a.base + to_string(a.r) + "-area", a.place + " (" + to_string(a.r) + " mi radius)",
					  't', 'a', (list<Region*>*)0, (list<HighwaySystem*>*)0, &a);
	}
	// write new graph_vector entries to disk
      #ifdef threading_enabled
	// set up for threaded subgraph generation
	for (unsigned int t = 0; t < args.numthreads; t++)
		thr[t] = new thread(SubgraphThread, t, &graph_data, &graph_vector, &graphnum, &list_mtx, args.graphfilepath + "/");
	for (unsigned int t = 0; t < args.numthreads; t++)
		thr[t]->join();
	for (unsigned int t = 0; t < args.numthreads; t++)
		delete thr[t];
      #else
	while (graphnum < graph_vector.size())
	{	graph_data.write_subgraphs_tmg(graph_vector, args.graphfilepath + "/", graphnum, 0);
		graphnum += 3;
	}
      #endif
	graph_types.push_back({"area", "Routes Within a Given Radius of a Place",
			       "These graphs contain all routes currently plotted within the given distance radius of the given place."});
	cout << '!' << endl;
	area_list.clear();


	// Graphs restricted by region
	cout << et.et() << "Creating regional data graphs." << endl;

	// We will create graph data and a graph file for each region that includes
	// any active or preview systems

	// add entries to graph_vector
	for (Region &region : all_regions)
	{	if (region.active_preview_mileage == 0) continue;
		regions = new list<Region*>(1, &region);
			  // deleted on termination of program
		graph_vector.emplace_back(region.code + "-region", region.name + " (" + region.type + ")",
					  's', 'r', regions, (list<HighwaySystem*>*)0, (PlaceRadius*)0);
		graph_vector.emplace_back(region.code + "-region", region.name + " (" + region.type + ")",
					  'c', 'r', (list<Region*>*)0, (list<HighwaySystem*>*)0, (PlaceRadius*)0);
		graph_vector.emplace_back(region.code + "-region", region.name + " (" + region.type + ")",
					  't', 'r', (list<Region*>*)0, (list<HighwaySystem*>*)0, (PlaceRadius*)0);
	}
	// write new graph_vector entries to disk
      #ifdef threading_enabled
	// set up for threaded subgraph generation
	for (unsigned int t = 0; t < args.numthreads; t++)
		thr[t] = new thread(SubgraphThread, t, &graph_data, &graph_vector, &graphnum, &list_mtx, args.graphfilepath + "/");
	for (unsigned int t = 0; t < args.numthreads; t++)
		thr[t]->join();
	for (unsigned int t = 0; t < args.numthreads; t++)
		delete thr[t];
      #else
	while (graphnum < graph_vector.size())
	{	graph_data.write_subgraphs_tmg(graph_vector, args.graphfilepath + "/", graphnum, 0);
		graphnum += 3;
	}
      #endif
	graph_types.push_back({"region", "Routes Within a Single Region", "These graphs contain all routes currently plotted within the given region."});
	cout << "!" << endl;


	// Graphs restricted by system - from systemgraphs.csv file
	cout << et.et() << "Creating system data graphs." << endl;

	// We will create graph data and a graph file for only a few interesting
	// systems, as many are not useful on their own
	HighwaySystem *h;
	filename = args.highwaydatapath+"/graphs/systemgraphs.csv";
	file.open(filename.data());
	getline(file, line);  // ignore header line

	// add entries to graph_vector
	while (getline(file, line))
	{	h = 0;
		for (HighwaySystem *hs : highway_systems)
		  if (hs->systemname == line)
		  {	h = hs;
			break;
		  }
		if (h)
		{	systems = new list<HighwaySystem*>(1, h);
				  // deleted on termination of program
			graph_vector.emplace_back(h->systemname + "-system", h->systemname + " (" + h->fullname + ")",
						  's', 's', (list<Region*>*)0, systems, (PlaceRadius*)0);
			graph_vector.emplace_back(h->systemname + "-system", h->systemname + " (" + h->fullname + ")",
						  'c', 's', (list<Region*>*)0, (list<HighwaySystem*>*)0, (PlaceRadius*)0);
			graph_vector.emplace_back(h->systemname + "-system", h->systemname + " (" + h->fullname + ")",
						  't', 's', (list<Region*>*)0, (list<HighwaySystem*>*)0, (PlaceRadius*)0);
		}
	}
	file.close();
	// write new graph_vector entries to disk
      #ifdef threading_enabled
	// set up for threaded subgraph generation
	for (unsigned int t = 0; t < args.numthreads; t++)
		thr[t] = new thread(SubgraphThread, t, &graph_data, &graph_vector, &graphnum, &list_mtx, args.graphfilepath + "/");
	for (unsigned int t = 0; t < args.numthreads; t++)
		thr[t]->join();
	for (unsigned int t = 0; t < args.numthreads; t++)
		delete thr[t];
      #else
	while (graphnum < graph_vector.size())
	{	graph_data.write_subgraphs_tmg(graph_vector, args.graphfilepath + "/", graphnum, 0);
		graphnum += 3;
	}
      #endif
	if (h)	graph_types.push_back({"system", "Routes Within a Single Highway System",
				       "These graphs contain the routes within a single highway system and are not restricted by region."});
	cout << "!" << endl;


	// Some additional interesting graphs, the "multisystem" graphs
	cout << et.et() << "Creating multisystem graphs." << endl;

	filename = args.highwaydatapath+"/graphs/multisystem.csv";
	file.open(filename.data());
	getline(file, line);  // ignore header line

	// add entries to graph_vector
	while (getline(file, line))
	{	vector<char*> fields;
		char *cline = new char[line.size()+1];
		strcpy(cline, line.data());
		for (char *token = strtok(cline, ";"); token; token = strtok(0, ";")) fields.push_back(token);
		if (fields.size() != 3)
		{	cout << "Could not parse multisystem.csv line: " << line << endl;
			delete[] cline;
			continue;
		}
		systems = new list<HighwaySystem*>;
			  // deleted on termination of program
		for(char* s = strtok(fields[2], ","); s; s = strtok(0, ","))
		  for (HighwaySystem *h : highway_systems)
		    if (s == h->systemname)
		    {	systems->push_back(h);
			break;
		    }
		graph_vector.emplace_back(fields[1], fields[0], 's', 'S', (list<Region*>*)0, systems, (PlaceRadius*)0);
		graph_vector.emplace_back(fields[1], fields[0], 'c', 'S', (list<Region*>*)0, (list<HighwaySystem*>*)0, (PlaceRadius*)0);
		graph_vector.emplace_back(fields[1], fields[0], 't', 'S', (list<Region*>*)0, (list<HighwaySystem*>*)0, (PlaceRadius*)0);
		delete[] cline;
	}
	file.close();
	// write new graph_vector entries to disk
      #ifdef threading_enabled
	// set up for threaded subgraph generation
	for (unsigned int t = 0; t < args.numthreads; t++)
		thr[t] = new thread(SubgraphThread, t, &graph_data, &graph_vector, &graphnum, &list_mtx, args.graphfilepath + "/");
	for (unsigned int t = 0; t < args.numthreads; t++)
		thr[t]->join();
	for (unsigned int t = 0; t < args.numthreads; t++)
		delete thr[t];
      #else
	while (graphnum < graph_vector.size())
	{	graph_data.write_subgraphs_tmg(graph_vector, args.graphfilepath + "/", graphnum, 0);
		graphnum += 3;
	}
      #endif
	graph_types.push_back({"multisystem", "Routes Within Multiple Highway Systems", "These graphs contain the routes within a set of highway systems."});
	cout << "!" << endl;


	// Some additional interesting graphs, the "multiregion" graphs
	cout << et.et() << "Creating multiregion graphs." << endl;

	filename = args.highwaydatapath+"/graphs/multiregion.csv";
	file.open(filename.data());
	getline(file, line);  // ignore header line

	// add entries to graph_vector
	while (getline(file, line))
	{	vector<char*> fields;
		char *cline = new char[line.size()+1];
		strcpy(cline, line.data());
		for (char *token = strtok(cline, ";"); token; token = strtok(0, ";")) fields.push_back(token);
		if (fields.size() != 3)
		{	cout << "Could not parse multiregion.csv line: " << line << endl;
			delete[] cline;
			continue;
		}
		regions = new list<Region*>;
			  // deleted on termination of program
		for(char* rg = strtok(fields[2], ","); rg; rg = strtok(0, ","))
		  for (Region &r : all_regions)
		    if (rg == r.code)
		    {	regions->push_back(&r);
			break;
		    }
		graph_vector.emplace_back(fields[1], fields[0], 's', 'R', regions, (list<HighwaySystem*>*)0, (PlaceRadius*)0);
		graph_vector.emplace_back(fields[1], fields[0], 'c', 'R', (list<Region*>*)0, (list<HighwaySystem*>*)0, (PlaceRadius*)0);
		graph_vector.emplace_back(fields[1], fields[0], 't', 'R', (list<Region*>*)0, (list<HighwaySystem*>*)0, (PlaceRadius*)0);
		delete[] cline;
	}
	file.close();
	// write new graph_vector entries to disk
      #ifdef threading_enabled
	// set up for threaded subgraph generation
	for (unsigned int t = 0; t < args.numthreads; t++)
		thr[t] = new thread(SubgraphThread, t, &graph_data, &graph_vector, &graphnum, &list_mtx, args.graphfilepath + "/");
	for (unsigned int t = 0; t < args.numthreads; t++)
		thr[t]->join();
	for (unsigned int t = 0; t < args.numthreads; t++)
		delete thr[t];
      #else
	while (graphnum < graph_vector.size())
	{	graph_data.write_subgraphs_tmg(graph_vector, args.graphfilepath + "/", graphnum, 0);
		graphnum += 3;
	}
      #endif
	graph_types.push_back({"multiregion", "Routes Within Multiple Regions", "These graphs contain the routes within a set of regions."});
	cout << "!" << endl;


	// country graphs - we find countries that have regions
	// that have routes with active or preview mileage
	cout << et.et() << "Creating country graphs." << endl;

	// add entries to graph_vector
	for (pair<string, string> &c : countries)
	{	regions = new list<Region*>;
			  // deleted on termination of program
		for (Region &r : all_regions)
		  // does it match this country and have routes?
		  if (&c == r.country && r.active_preview_mileage)
		    regions->push_back(&r);
		// does it have at least two?  if none, no data,
		// if 1 we already generated a graph for that one region
		if (regions->size() < 2) delete regions;
		else {	graph_vector.emplace_back(c.first + "-country", c.second + " All Routes in Country",
						  's', 'c', regions, (list<HighwaySystem*>*)0, (PlaceRadius*)0);
			graph_vector.emplace_back(c.first + "-country", c.second + " All Routes in Country",
						  'c', 'c', (list<Region*>*)0, (list<HighwaySystem*>*)0, (PlaceRadius*)0);
			graph_vector.emplace_back(c.first + "-country", c.second + " All Routes in Country",
						  't', 't', (list<Region*>*)0, (list<HighwaySystem*>*)0, (PlaceRadius*)0);
		     }
	}
	// write new graph_vector entries to disk
      #ifdef threading_enabled
	// set up for threaded subgraph generation
	for (unsigned int t = 0; t < args.numthreads; t++)
		thr[t] = new thread(SubgraphThread, t, &graph_data, &graph_vector, &graphnum, &list_mtx, args.graphfilepath + "/");
	for (unsigned int t = 0; t < args.numthreads; t++)
		thr[t]->join();
	for (unsigned int t = 0; t < args.numthreads; t++)
		delete thr[t];
      #else
	while (graphnum < graph_vector.size())
	{	graph_data.write_subgraphs_tmg(graph_vector, args.graphfilepath + "/", graphnum, 0);
		graphnum += 3;
	}
      #endif
	graph_types.push_back({"country", "Routes Within a Single Multi-Region Country",
			       "These graphs contain the routes within a single country that is composed of multiple regions that contain plotted routes.  " + 
			       string("Countries consisting of a single region are represented by their regional graph.")});
	cout << "!" << endl;


	// continent graphs -- any continent with data will be created
	cout << et.et() << "Creating continent graphs." << endl;

	// add entries to graph_vector
	for (pair<string, string> &c : continents)
	{	regions = new list<Region*>;
			  // deleted on termination of program
		for (Region &r : all_regions)
		  // does it match this continent and have routes?
		  if (&c == r.continent && r.active_preview_mileage)
		    regions->push_back(&r);
		// generate for any continent with at least 1 region with mileage
		if (regions->size() < 1) delete regions;
		else {	graph_vector.emplace_back(c.first + "-continent", c.second + " All Routes on Continent",
						  's', 'C', regions, (list<HighwaySystem*>*)0, (PlaceRadius*)0);
			graph_vector.emplace_back(c.first + "-continent", c.second + " All Routes on Continent",
						  'c', 'C', (list<Region*>*)0, (list<HighwaySystem*>*)0, (PlaceRadius*)0);
			graph_vector.emplace_back(c.first + "-continent", c.second + " All Routes on Continent",
						  't', 'C', (list<Region*>*)0, (list<HighwaySystem*>*)0, (PlaceRadius*)0);
		     }
	}
	// write new graph_vector entries to disk
      #ifdef threading_enabled
	// set up for threaded subgraph generation
	for (unsigned int t = 0; t < args.numthreads; t++)
		thr[t] = new thread(SubgraphThread, t, &graph_data, &graph_vector, &graphnum, &list_mtx, args.graphfilepath + "/");
	for (unsigned int t = 0; t < args.numthreads; t++)
		thr[t]->join();
	for (unsigned int t = 0; t < args.numthreads; t++)
		delete thr[t];
      #else
	while (graphnum < graph_vector.size())
	{	graph_data.write_subgraphs_tmg(graph_vector, args.graphfilepath + "/", graphnum, 0);
		graphnum += 3;
	}
      #endif
	graph_types.push_back({"continent", "Routes Within a Continent", "These graphs contain the routes on a continent."});
	cout << "!" << endl;
     }

graph_data.clear();
