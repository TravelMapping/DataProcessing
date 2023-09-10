// Graphs restricted by system - from systemgraphs.csv file
#ifndef threading_enabled
cout << et.et() << "Creating system data graphs." << endl;
#endif
// We will create graph data and a graph file for only a few interesting
// systems, as many are not useful on their own
file.open(Args::datapath+"/graphs/systemgraphs.csv");
getline(file, line);  // ignore header line

// add entries to graph vector
while (getline(file, line))
{	for (h = HighwaySystem::syslist.data; h < sys_end; h++)
	  if (h->systemname == line)
	  {	GraphListEntry::add_group(
			h->systemname + "-system",
			h->systemname + " (" + h->fullname + ")",
			's', nullptr, new list<HighwaySystem*>(1, h), nullptr);
				      // deleted @ end of HighwayGraph::write_subgraphs_tmg
		break;
	  }
	if (h == sys_end)
		el.add_error("unrecognized system code "+line+" in systemgraphs.csv");
}
file.close();
#ifndef threading_enabled
// write new graph vector entries to disk
while (GraphListEntry::num < GraphListEntry::entries.size())
{	graph_data.write_subgraphs_tmg(GraphListEntry::num, 0, &all_waypoints, &et, &term_mtx);
	GraphListEntry::num += 3;
}
cout << "!" << endl;
#endif
graph_types.push_back({"system", "Routes Within a Single Highway System",
		       "These graphs contain the routes within a single highway system and are not restricted by region."});
