// Graphs restricted by system - from systemgraphs.csv file
#ifndef threading_enabled
cout << et.et() << "Creating system data graphs." << endl;
#endif
// We will create graph data and a graph file for only a few interesting
// systems, as many are not useful on their own
HighwaySystem *h;
file.open( args.highwaydatapath+"/graphs/systemgraphs.csv");
getline(file, line);  // ignore header line

// add entries to graph_vector
while (getline(file, line))
{	h = 0;
	for (HighwaySystem *hs : highway_systems)
	  if (hs->systemname == line)
	  {	h = hs;
		break;
	  }
	if (h)
	{	systems = new list<HighwaySystem*>(1, h);
			  // deleted on termination of program
		graph_vector.emplace_back(h->systemname + "-system", h->systemname + " (" + h->fullname + ")",
					  's', 's', (list<Region*>*)0, systems, (PlaceRadius*)0);
		graph_vector.emplace_back(h->systemname + "-system", h->systemname + " (" + h->fullname + ")",
					  'c', 's', (list<Region*>*)0, (list<HighwaySystem*>*)0, (PlaceRadius*)0);
		graph_vector.emplace_back(h->systemname + "-system", h->systemname + " (" + h->fullname + ")",
					  't', 's', (list<Region*>*)0, (list<HighwaySystem*>*)0, (PlaceRadius*)0);
	}
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
if (h)	graph_types.push_back({"system", "Routes Within a Single Highway System",
			       "These graphs contain the routes within a single highway system and are not restricted by region."});
