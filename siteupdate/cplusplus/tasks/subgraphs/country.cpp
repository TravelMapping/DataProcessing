// country graphs - we find countries that have regions with active or preview mileage
#ifndef threading_enabled
cout << et.et() << "Creating country graphs." << endl;
#endif
// add entries to graph vector
for (auto c = Region::countries.data(), dummy = c+Region::countries.size()-1; c < dummy; c++)
{	regions = new list<Region*>;
		  // deleted @ end of HighwayGraph::write_subgraphs_tmg
	for (Region& r : Region::allregions)
	  // does it match this country and have routes?
	  if (c == r.country && r.active_preview_mileage)
	    regions->push_back(&r);
	// does it have at least two?  if none, no data; if 1 we already generated a graph for that one region
	if (regions->size() < 2) delete regions;
	else {	GraphListEntry::add_group(
			c->first + "-country",
			c->second + " All Routes in Country",
			'c', regions, nullptr, nullptr);
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
graph_types.push_back({"country", "Routes Within a Single Multi-Region Country",
		       "These graphs contain the routes within a single country that is composed of multiple regions that contain plotted routes.  " + 
		       string("Countries consisting of a single region are represented by their regional graph.")});
