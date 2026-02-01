// Some additional interesting graphs, the "multisystem" graphs
file.open(Args::datapath+"/graphs/multisystem.csv");
getline(file, line);  // ignore header line

while (getline(file, line))
{	if (line.empty()) continue;
	vector<char*> fields;
	char *cline = new char[line.size()+1];
		      // deleted @ end of this while loop after tokens are processed
	strcpy(cline, line.data());
	for (char *token = strtok(cline, ";"); token; token = strtok(0, ";")) fields.push_back(token);
	if (fields.size() != 3)
	{	el.add_error("Could not parse multisystem.csv line: [" + line
			   + "], expected 3 fields, found " + std::to_string(fields.size()));
		delete[] cline;
		continue;
	}
	if (fields[1]-fields[0]-1 > DBFieldLength::graphDescr)
	  el.add_error("description > " + std::to_string(DBFieldLength::graphDescr)
		     + " bytes in multisystem.csv line: " + line);
	if (fields[2]-fields[1] > DBFieldLength::graphFilename-13)
	  el.add_error("title > " + std::to_string(DBFieldLength::graphFilename-14)
		     + " bytes in multisystem.csv line: " + line);
	systems = new vector<HighwaySystem*>;
		  // deleted @ end of HighwayGraph::write_subgraphs_tmg
		  // or when aborting due to ErrorList errors
	for(char* s = strtok(fields[2], ","); s; s = strtok(0, ","))
	  try {	HighwaySystem* const h = HighwaySystem::sysname_hash.at(s);
		if (h->active_or_preview())
		{	systems->push_back(h);
			h->is_subgraph_system = 1;
		} else	el.add_error("devel system "+h->systemname+" in multisystem.csv line: "+line);
	      }
	  catch (const std::out_of_range&)
	      {	el.add_error("unrecognized system code "+string(s)+" in multisystem.csv line: "+line);
	      }
	if (systems->size()) GraphListEntry::add_group(fields[1], fields[0], 'S', nullptr, systems, nullptr);
	else delete systems;
	delete[] cline;
}
file.close();
graph_types.push_back({"multisystem", "Routes Within Multiple Highway Systems", "These graphs contain the routes within a set of highway systems."});
