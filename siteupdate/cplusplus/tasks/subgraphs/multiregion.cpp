// Some additional interesting graphs, the "multiregion" graphs
file.open(Args::datapath+"/graphs/multiregion.csv");
getline(file, line);  // ignore header line

while (getline(file, line))
{	if (line.empty()) continue;
	vector<char*> fields;
	char *cline = new char[line.size()+1];
		      // deleted @ end of this while loop after tokens are processed
	strcpy(cline, line.data());
	for (char *token = strtok(cline, ";"); token; token = strtok(0, ";")) fields.push_back(token);
	if (fields.size() != 3)
	{	el.add_error("Could not parse multiregion.csv line: [" + line
			   + "], expected 3 fields, found " + std::to_string(fields.size()));
		delete[] cline;
		continue;
	}
	if (fields[1]-fields[0]-1 > DBFieldLength::graphDescr)
	  el.add_error("description > " + std::to_string(DBFieldLength::graphDescr)
		     + " bytes in multiregion.csv line: " + line);
	if (fields[2]-fields[1] > DBFieldLength::graphFilename-13)
	  el.add_error("title > " + std::to_string(DBFieldLength::graphFilename-14)
		     + " bytes in multiregion.csv line: " + line);
	regions = new vector<Region*>;
		  // deleted @ end of HighwayGraph::write_subgraphs_tmg
	for(char* rg = strtok(fields[2], ","); rg; rg = strtok(0, ","))
	  try {	regions->push_back(Region::code_hash.at(rg));
	      }
	  catch (const out_of_range&)
	      {	el.add_error("unrecognized region code "+string(rg)+" in multiregion.csv line: "+line);
	      }
	if (regions->size()) GraphListEntry::add_group(fields[1], fields[0], 'R', regions, nullptr, nullptr, el);
	else delete regions;
	delete[] cline;
}
file.close();
graph_types.push_back({"multiregion", "Routes Within Multiple Regions", "These graphs contain the routes within a set of regions."});
