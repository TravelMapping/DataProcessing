// continent graphs -- any continent with data will be created
#ifndef threading_enabled
cout << et.et() << "Creating continent graphs." << endl;
#endif
// add entries to graph_vector
for (size_t c = 0; c < continents.size()-1; c++)
{	regions = new list<Region*>;
		  // deleted on termination of program
	for (Region* r : all_regions)
	  // does it match this continent and have routes?
	  if (&continents[c] == r->continent && r->active_preview_mileage)
	    regions->push_back(r);
	// generate for any continent with at least 1 region with mileage
	if (regions->size() < 1) delete regions;
	else {	graph_vector.emplace_back(continents[c].first + "-continent", continents[c].second + " All Routes on Continent",
					  's', 'C', regions, (list<HighwaySystem*>*)0, (PlaceRadius*)0);
		graph_vector.emplace_back(continents[c].first + "-continent", continents[c].second + " All Routes on Continent",
					  'c', 'C', (list<Region*>*)0, (list<HighwaySystem*>*)0, (PlaceRadius*)0);
		graph_vector.emplace_back(continents[c].first + "-continent", continents[c].second + " All Routes on Continent",
					  't', 'C', (list<Region*>*)0, (list<HighwaySystem*>*)0, (PlaceRadius*)0);
	     }
}
#ifndef threading_enabled
// write new graph_vector entries to disk
while (graphnum < graph_vector.size())
{	graph_data.write_subgraphs_tmg(graph_vector, args.graphfilepath + "/", graphnum, 0, &all_waypoints, &et);
	graphnum += 3;
}
cout << "!" << endl;
#endif
graph_types.push_back({"continent", "Routes Within a Continent", "These graphs contain the routes on a continent."});
