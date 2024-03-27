// region graphs - for each region with active or preview mileage
for (Region& region : Region::allregions)
{	if (region.active_preview_mileage == 0) continue;
	GraphListEntry::add_group(
		region.code + "-region",
		region.name + " (" + region.type + ")", 'r',
		new list<Region*>(1, &region), nullptr, nullptr);
		// deleted @ end of HighwayGraph::write_subgraphs_tmg
}
graph_types.push_back({"region", "Routes Within a Single Region", "These graphs contain all routes currently plotted within the given region."});
