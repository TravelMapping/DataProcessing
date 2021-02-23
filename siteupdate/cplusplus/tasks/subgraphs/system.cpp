#define ADDGRAPH(F) GraphListEntry::entries.emplace_back(\
	h->systemname + "-system", h->systemname + " (" + h->fullname + ")", F, 's', (list<Region*>*)0, systems, (PlaceRadius*)0)
// Graphs restricted by system - from systemgraphs.csv file
#ifndef threading_enabled
cout << et.et() << "Creating system data graphs." << endl;
#endif
// We will create graph data and a graph file for only a few interesting
// systems, as many are not useful on their own
HighwaySystem *h;
file.open(Args::highwaydatapath+"/graphs/systemgraphs.csv");
getline(file, line);  // ignore header line

// add entries to graph vector
while (getline(file, line))
{	h = 0;
	for (HighwaySystem *hs : HighwaySystem::syslist)
	  if (hs->systemname == line)
	  {	h = hs;
		break;
	  }
	if (h)
	{	systems = new list<HighwaySystem*>(1, h);
			  // deleted on termination of program
		ADDGRAPH('s');
		ADDGRAPH('c');
		ADDGRAPH('t');
		#undef ADDGRAPH
	}
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
if (h)	graph_types.push_back({"system", "Routes Within a Single Highway System",
			       "These graphs contain the routes within a single highway system and are not restricted by region."});
