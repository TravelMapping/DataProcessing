// continent graphs - any continent with active or preview mileage will be created
for (auto c = Region::continents.data(), dummy = c+Region::continents.size()-1; c < dummy; c++)
{	regions = new vector<Region*>;
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
			'C', regions, nullptr, nullptr, el);
	     }
}
graph_types.push_back({"continent", "Routes Within a Continent", "These graphs contain the routes on a continent."});
