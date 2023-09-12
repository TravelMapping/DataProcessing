// graphs restricted by place/area - from areagraphs.csv file
#ifndef threading_enabled
cout << et.et() << "Creating area data graphs." << endl;
#endif
file.open(Args::datapath+"/graphs/areagraphs.csv");
getline(file, line);  // ignore header line

// add entries to graph vector
while (getline(file, line))
{	if (line.empty()) continue;
	vector<char*> fields;
	char *cline = new char[line.size()+1];
		      // deleted @ end of this while loop after tokens are processed
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
	double r = strtod(fields[4], &endptr);
	if (*endptr || r <= 0) {el.add_error("invalid radius in areagraphs.csv line: " + line); r=1;}
	PlaceRadius *a = new PlaceRadius(fields[0], fields[1], lat, lng, r);
			 // deleted @ end of HighwayGraph::write_subgraphs_tmg
	GraphListEntry::add_group(
		string(fields[1]) + fields[4] + "-area",
		string(fields[0]) + " (" + fields[4] + " mi radius)",
		'a', nullptr, nullptr, a);
	delete[] cline;
}
file.close();
#ifndef threading_enabled
// write new graph vector entries to disk
while (GraphListEntry::num < GraphListEntry::entries.size())
{	graph_data.write_subgraphs_tmg(GraphListEntry::num, 0, &all_waypoints, &et, &term_mtx);
	GraphListEntry::num += 3;
}
cout << '!' << endl;
#endif
graph_types.push_back({"area", "Routes Within a Given Radius of a Place",
		       "These graphs contain all routes currently plotted within the given distance radius of the given place."});
