// country graphs - we find countries that have regions
// that have routes with active or preview mileage
#ifndef threading_enabled
cout << et.et() << "Creating country graphs." << endl;
#endif
// add entries to graph_vector
for (size_t c = 0; c < countries.size()-1; c++)
{	regions = new list<Region*>;
		  // deleted on termination of program
	for (Region &r : all_regions)
	  // does it match this country and have routes?
	  if (&countries[c] == r.country && r.active_preview_mileage)
	    regions->push_back(&r);
	// does it have at least two?  if none, no data,
	// if 1 we already generated a graph for that one region
	if (regions->size() < 2) delete regions;
	else {	graph_vector.emplace_back(countries[c].first + "-country", countries[c].second + " All Routes in Country",
					  's', 'c', regions, (list<HighwaySystem*>*)0, (PlaceRadius*)0);
		graph_vector.emplace_back(countries[c].first + "-country", countries[c].second + " All Routes in Country",
					  'c', 'c', (list<Region*>*)0, (list<HighwaySystem*>*)0, (PlaceRadius*)0);
		graph_vector.emplace_back(countries[c].first + "-country", countries[c].second + " All Routes in Country",
					  't', 'c', (list<Region*>*)0, (list<HighwaySystem*>*)0, (PlaceRadius*)0);
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
graph_types.push_back({"country", "Routes Within a Single Multi-Region Country",
		       "These graphs contain the routes within a single country that is composed of multiple regions that contain plotted routes.  " + 
		       string("Countries consisting of a single region are represented by their regional graph.")});
