// Some additional interesting graphs, the "multisystem" graphs
#ifndef threading_enabled
cout << et.et() << "Creating multisystem graphs." << endl;
#endif
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
	graph_vector.emplace_back(fields[1], fields[0],
				  's', 'S', (list<Region*>*)0, systems, (PlaceRadius*)0);
	graph_vector.emplace_back(fields[1], fields[0],
				  'c', 'S', (list<Region*>*)0, (list<HighwaySystem*>*)0, (PlaceRadius*)0);
	graph_vector.emplace_back(fields[1], fields[0],
				  't', 'S', (list<Region*>*)0, (list<HighwaySystem*>*)0, (PlaceRadius*)0);
	delete[] cline;
}
file.close();
#ifndef threading_enabled
// write new graph_vector entries to disk
while (graphnum < graph_vector.size())
{	graph_data.write_subgraphs_tmg(graph_vector, args.graphfilepath + "/", graphnum, 0, &all_waypoints, &et);
	graphnum += 3;
}
cout << "!" << endl;
#endif
graph_types.push_back({"multisystem", "Routes Within Multiple Highway Systems", "These graphs contain the routes within a set of highway systems."});
