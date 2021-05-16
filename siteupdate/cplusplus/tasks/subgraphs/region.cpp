#define ADDGRAPH(F) GraphListEntry::entries.emplace_back(\
	region->code + "-region", region->name + " (" + region->type + ")", F, 'r', regions, (list<HighwaySystem*>*)0, (PlaceRadius*)0)
// Graphs restricted by region
#ifndef threading_enabled
cout << et.et() << "Creating regional data graphs." << endl;
#endif
// We will create graph data and a graph file for each region that includes
// any active or preview systems

// add entries to graph vector
for (Region* region : Region::allregions)
{	if (region->active_preview_mileage == 0) continue;
	regions = new list<Region*>(1, region);
		  // deleted on termination of program
	ADDGRAPH('s');
	ADDGRAPH('c');
	ADDGRAPH('t');
	#undef ADDGRAPH
}
#ifndef threading_enabled
// write new graph vector entries to disk
while (GraphListEntry::num < GraphListEntry::entries.size())
{	graph_data.write_subgraphs_tmg(GraphListEntry::num, 0, &all_waypoints, &et, &term_mtx);
	GraphListEntry::num += 3;
}
cout << "!" << endl;
#endif
graph_types.push_back({"region", "Routes Within a Single Region", "These graphs contain all routes currently plotted within the given region."});
