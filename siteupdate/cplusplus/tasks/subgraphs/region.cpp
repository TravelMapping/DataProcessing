// Graphs restricted by region
#ifndef threading_enabled
cout << et.et() << "Creating regional data graphs." << endl;
#endif
// We will create graph data and a graph file for each region that includes
// any active or preview systems

// add entries to graph vector
for (Region& region : Region::allregions)
{	if (region.active_preview_mileage == 0) continue;
	GraphListEntry::add_group(
		region.code + "-region",
		region.name + " (" + region.type + ")", 'r',
		new list<Region*>(1, &region), nullptr, nullptr);
		// deleted @ end of HighwayGraph::write_subgraphs_tmg
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
