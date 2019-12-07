// Some additional interesting graphs, the "multiregion" graphs
#ifndef threading_enabled
cout << et.et() << "Creating multiregion graphs." << endl;
#endif
file.open(args.highwaydatapath+"/graphs/multiregion.csv");
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
	graph_vector.emplace_back(fields[1], fields[0],
				  's', 'R', regions, (list<HighwaySystem*>*)0, (PlaceRadius*)0);
	graph_vector.emplace_back(fields[1], fields[0],
				  'c', 'R', (list<Region*>*)0, (list<HighwaySystem*>*)0, (PlaceRadius*)0);
	graph_vector.emplace_back(fields[1], fields[0],
				  't', 'R', (list<Region*>*)0, (list<HighwaySystem*>*)0, (PlaceRadius*)0);
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
graph_types.push_back({"multiregion", "Routes Within Multiple Regions", "These graphs contain the routes within a set of regions."});
