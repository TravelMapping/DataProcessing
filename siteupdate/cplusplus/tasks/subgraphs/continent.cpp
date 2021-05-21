#define ADDGRAPH(F) GraphListEntry::entries.emplace_back(\
	continents[c].first + "-continent", continents[c].second + " All Routes on Continent", \
	F, 'C', regions, (list<HighwaySystem*>*)0, (PlaceRadius*)0)
// continent graphs -- any continent with data will be created
#ifndef threading_enabled
cout << et.et() << "Creating continent graphs." << endl;
#endif
// add entries to graph vector
for (size_t c = 0; c < continents.size()-1; c++)
{	regions = new list<Region*>;
		  // deleted on termination of program
	for (Region* r : Region::allregions)
	  // does it match this continent and have routes?
	  if (&continents[c] == r->continent && r->active_preview_mileage)
	    regions->push_back(r);
	// generate for any continent with at least 1 region with mileage
	if (regions->size() < 1) delete regions;
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
graph_types.push_back({"continent", "Routes Within a Continent", "These graphs contain the routes on a continent."});
