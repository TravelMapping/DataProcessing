// graphs restricted by place/area - from areagraphs.csv file
#ifndef threading_enabled
cout << et.et() << "Creating area data graphs." << endl;
#endif
file.open(args.highwaydatapath+"/graphs/areagraphs.csv");
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
#ifndef threading_enabled
// write new graph_vector entries to disk
while (graphnum < graph_vector.size())
{	graph_data.write_subgraphs_tmg(graph_vector, args.graphfilepath + "/", graphnum, 0, &all_waypoints, &et);
	graphnum += 3;
}
cout << '!' << endl;
#endif
graph_types.push_back({"area", "Routes Within a Given Radius of a Place",
		       "These graphs contain all routes currently plotted within the given distance radius of the given place."});
