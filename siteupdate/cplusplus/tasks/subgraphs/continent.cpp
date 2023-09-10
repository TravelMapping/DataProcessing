// continent graphs -- any continent with active or preview mileage will be created
#ifndef threading_enabled
cout << et.et() << "Creating continent graphs." << endl;
#endif
// add entries to graph vector
for (auto c = Region::continents.data(), dummy = c+Region::continents.size()-1; c < dummy; c++)
{	regions = new list<Region*>;
		  // deleted @ end of HighwayGraph::write_subgraphs_tmg
	for (Region& r : Region::allregions)
	  // does it match this continent and have routes?
	  if (c == r.continent && r.active_preview_mileage)
	    regions->push_back(&r);
	// generate for any continent with at least 1 region with mileage
	if (regions->size() < 1) delete regions;
	else {	GraphListEntry::add_group(
			c->first + "-continent",
			c->second + " All Routes on Continent",
			'C', regions, nullptr, nullptr);
	     }
}
#ifndef threading_enabled
// write new graph vector entries to disk
while (GraphListEntry::num < GraphListEntry::entries.size())
{	graph_data.write_subgraphs_tmg(GraphListEntry::num, 0, &all_waypoints, &et, &term_mtx);
	GraphListEntry::num += 3;
}
cout << "!" << endl;
#endif
graph_types.push_back({"continent", "Routes Within a Continent", "These graphs contain the routes on a continent."});
