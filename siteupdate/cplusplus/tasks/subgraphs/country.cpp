#define ADDGRAPH(F) GraphListEntry::entries.emplace_back(\
	countries[c].first + "-country", countries[c].second + " All Routes in Country", F, 'c', regions, (list<HighwaySystem*>*)0, (PlaceRadius*)0)
// country graphs - we find countries that have regions
// that have routes with active or preview mileage
#ifndef threading_enabled
cout << et.et() << "Creating country graphs." << endl;
#endif
// add entries to graph vector
for (size_t c = 0; c < countries.size()-1; c++)
{	regions = new list<Region*>;
		  // deleted on termination of program
	for (Region* r : Region::allregions)
	  // does it match this country and have routes?
	  if (&countries[c] == r->country && r->active_preview_mileage)
	    regions->push_back(r);
	// does it have at least two?  if none, no data,
	// if 1 we already generated a graph for that one region
	if (regions->size() < 2) delete regions;
	else {	ADDGRAPH('s');
		ADDGRAPH('c');
		ADDGRAPH('t');
		#undef ADDGRAPH
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
