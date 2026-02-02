// fully customizable graphs using any combination of PlaceRadius, region(s) & system(s)
file.open(Args::datapath+"/graphs/fullcustom.csv");
if (file.is_open())
{	getline(file, line);  // ignore header line
	while (getline(file, line))
	{	// parse fullcustom.csv line
		if (line.empty()) continue;
		size_t NumFields = 7;
		std::string descr, root, lat_s, lng_s, radius, regionlist, systemlist;
		std::string* fields[7] = {&descr, &root, &lat_s, &lng_s, &radius, &regionlist, &systemlist};
		split(line, fields, NumFields, ';');
		if (NumFields != 7)
		{	el.add_error("Could not parse fullcustom.csv line: [" + line
				   + "], expected 7 fields, found " + std::to_string(NumFields));
			continue;
		}
		if (descr.size() > DBFieldLength::graphDescr)
		  el.add_error("description > " + std::to_string(DBFieldLength::graphDescr)
			     + " bytes in fullcustom.csv line: " + line);
		if (root.size() > DBFieldLength::graphFilename-14)
		  el.add_error("title > " + std::to_string(DBFieldLength::graphFilename-14)
			     + " bytes in fullcustom.csv line: " + line);
		bool ok = 1;

		// 3 columns of PlaceRadius data
		PlaceRadius* a = 0;
		char blanks = lat_s.empty() + lng_s.empty() + radius.empty();
		if (!blanks)
		{	// convert numeric fields
			char* endptr;
			double lat = strtod(lat_s.data(), &endptr);
			if (*endptr)		{el.add_error("invalid lat in fullcustom.csv line: " + line); ok = 0;}
			double lng = strtod(lng_s.data(), &endptr);
			if (*endptr)		{el.add_error("invalid lng in fullcustom.csv line: " + line); ok = 0;}
			double r = strtod(radius.data(), &endptr);
			if (*endptr || r <= 0)	{el.add_error("invalid radius in fullcustom.csv line: " + line); ok = 0;}
			a = new PlaceRadius(descr.data(), root.data(), lat, lng, r);
		}	    // deleted @ end of HighwayGraph::write_subgraphs_tmg or when aborting due to ErrorList errors
		else if (blanks != 3)
		{	el.add_error("lat/lng/radius error in fullcustom.csv line: [" + line
				   + "], either all or none must be populated");
			ok = 0;
		}
		else if (regionlist.empty() && systemlist.empty())
		{	el.add_error("Disallowed full custom graph in line: [" + line
				   + "], functionally identical to tm-master");
			continue;
		}

		// regionlist
		if (regionlist.empty()) regions = 0;
		else {	regions = new vector<Region*>;
				  // deleted @ end of HighwayGraph::write_subgraphs_tmg
				  // or when aborting due to ErrorList errors
			char* field = new char[regionlist.size()+1];
				      // deleted once region tokens are processed
			strcpy(field, regionlist.data());
			for(char* rg = strtok(field, ","); rg; rg = strtok(0, ","))
			  try {	regions->push_back(Region::code_hash.at(rg));
			      }
			  catch (const std::out_of_range& oor)
			      {	el.add_error("unrecognized region code "+string(rg)+" in fullcustom.csv line: "+line);
				ok = 0;
			      }
			delete[] field;
		     }

		// systemlist
		if (systemlist.empty()) systems = 0;
		else {	systems = new vector<HighwaySystem*>;
				  // deleted @ end of HighwayGraph::write_subgraphs_tmg
				  // or when aborting due to ErrorList errors
			char* field = new char[systemlist.size()+1];
				      // deleted once system tokens are processed
			strcpy(field, systemlist.data());
			for(char* s = strtok(field, ","); s; s = strtok(0, ","))
			  try {	HighwaySystem* const h = HighwaySystem::sysname_hash.at(s);
				if (h->active_or_preview())
				{	systems->push_back(h);
					h->is_subgraph_system = 1;
				} else	el.add_error("devel system "+h->systemname+" in fullcustom.csv line: "+line);
			      }
			  catch (const std::out_of_range&)
			      {	el.add_error("unrecognized system code "+string(s)+" in fullcustom.csv line: "+line);
				ok = 0;
			      }
			delete[] field;
		     }
		if (ok)	GraphListEntry::add_group(std::move(root), std::move(descr), 'f', regions, systems, a, el);
		else {	delete regions;
			delete systems;
			delete a;
		     }
	}
	file.close();
	graph_types.push_back({"fullcustom", "Full Custom Graphs",
	"These graphs can be restricted by any combination of one more more regions and one or more highway systems, and optionally within the given distance radius of a given place."});
}
