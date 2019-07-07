// Graphs restricted by region
#ifndef threading_enabled
cout << et.et() << "Creating regional data graphs." << endl;
#endif
// We will create graph data and a graph file for each region that includes
// any active or preview systems

// add entries to graph_vector
for (Region &region : all_regions)
{	if (region.active_preview_mileage == 0) continue;
	regions = new list<Region*>(1, &region);
		  // deleted on termination of program
	graph_vector.emplace_back(region.code + "-region", region.name + " (" + region.type + ")",
				  's', 'r', regions, (list<HighwaySystem*>*)0, (PlaceRadius*)0);
	graph_vector.emplace_back(region.code + "-region", region.name + " (" + region.type + ")",
				  'c', 'r', (list<Region*>*)0, (list<HighwaySystem*>*)0, (PlaceRadius*)0);
	graph_vector.emplace_back(region.code + "-region", region.name + " (" + region.type + ")",
				  't', 'r', (list<Region*>*)0, (list<HighwaySystem*>*)0, (PlaceRadius*)0);
}
#ifndef threading_enabled
// write new graph_vector entries to disk
while (graphnum < graph_vector.size())
{	graph_data.write_subgraphs_tmg(graph_vector, args.graphfilepath + "/", graphnum, 0, &all_waypoints, &et);
	graphnum += 3;
}
cout << "!" << endl;
#endif
graph_types.push_back({"region", "Routes Within a Single Region", "These graphs contain all routes currently plotted within the given region."});
