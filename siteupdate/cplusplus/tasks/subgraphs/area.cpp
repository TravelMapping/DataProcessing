// graphs restricted by place/area - from areagraphs.csv file
#ifndef threading_enabled
cout << et.et() << "Creating area data graphs." << endl;
#endif
file.open(args.highwaydatapath+"/graphs/areagraphs.csv");
getline(file, line);  // ignore header line
list<PlaceRadius> area_list;
while (getline(file, line))
{	if (line.empty()) continue;
	vector<char*> fields;
	char *cline = new char[line.size()+1];
	strcpy(cline, line.data());
	for (char *token = strtok(cline, ";"); token; token = strtok(0, ";")) fields.push_back(token);
	if (fields.size() != 5)
	{	el.add_error("Could not parse areagraphs.csv line: [" + line
			   + "], expected 5 fields, found " + std::to_string(fields.size()));
		delete[] cline;
		continue;
	}
	if (fields[1] + line.size()-fields[4] > DBFieldLength::graphDescr-12)
	  el.add_error("description + radius is too long by "
		     + std::to_string(fields[1] + line.size()-fields[4] - DBFieldLength::graphDescr+12)
		     + " byte(s) in areagraphs.csv line: " + line);
	if (fields[0]-fields[1]+fields[2]-fields[4]+line.size() > DBFieldLength::graphFilename-18)
	  el.add_error("title + radius = filename too long by "
		     + std::to_string(line.size()+fields[0]-fields[1]+fields[2]-fields[4] - DBFieldLength::graphFilename+18)
		     + " byte(s) in areagraphs.csv line: " + line);
	// convert numeric fields
	char* endptr;
	double lat = strtod(fields[2], &endptr);
	if (*endptr)		el.add_error("invalid lat in areagraphs.csv line: " + line);
	double lng = strtod(fields[3], &endptr);
	if (*endptr)		el.add_error("invalid lng in areagraphs.csv line: " + line);
	int r = strtol(fields[4], &endptr, 10);
	if (*endptr || r <= 0) {el.add_error("invalid radius in areagraphs.csv line: " + line); r=1;}
	area_list.emplace_back(fields[0], fields[1], lat, lng, r);
	delete[] cline;
}
file.close();

// add entries to graph_vector
for (PlaceRadius &a : area_list)
{	graph_vector.emplace_back(a.title + to_string(a.r) + "-area", a.descr + " (" + to_string(a.r) + " mi radius)",
				  's', 'a', (list<Region*>*)0, (list<HighwaySystem*>*)0, &a);
	graph_vector.emplace_back(a.title + to_string(a.r) + "-area", a.descr + " (" + to_string(a.r) + " mi radius)",
				  'c', 'a', (list<Region*>*)0, (list<HighwaySystem*>*)0, &a);
	graph_vector.emplace_back(a.title + to_string(a.r) + "-area", a.descr + " (" + to_string(a.r) + " mi radius)",
				  't', 'a', (list<Region*>*)0, (list<HighwaySystem*>*)0, &a);
}
#ifndef threading_enabled
// write new graph_vector entries to disk
while (graphnum < graph_vector.size())
{	graph_data.write_subgraphs_tmg(graph_vector, args.graphfilepath + "/", graphnum, 0, &all_waypoints, &et, &term_mtx);
	graphnum += 3;
}
cout << '!' << endl;
#endif
graph_types.push_back({"area", "Routes Within a Given Radius of a Place",
		       "These graphs contain all routes currently plotted within the given distance radius of the given place."});
