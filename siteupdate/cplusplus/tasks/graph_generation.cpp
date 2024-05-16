cout << et.et() << "Setting up for graphs of highway data." << endl;
HighwayGraph graph_data(all_waypoints, et);

cout << et.et() << "Writing graph waypoint simplification log." << endl;
ofstream wslogfile(Args::logfilepath + "/waypointsimplification.log");
for (string& line : graph_data.waypoint_naming_log)
	wslogfile << line << '\n';
wslogfile.close();
graph_data.waypoint_naming_log.clear();

// start generating graphs and making entries for graph DB table
{	// Let's keep these braces here, for easily commenting out subgraph generation when developing waypoint simplification routines
	GraphListEntry::num = 3;

	cout << et.et() << "Writing master TM graph files." << endl;
	// print summary info
	std::cout << "   Simple graph has " << graph_data.vertices.size() << " vertices, " << graph_data.se << " edges." << std::endl;
	std::cout << "Collapsed graph has " << graph_data.cv << " vertices, " << graph_data.ce << " edges." << std::endl;
	std::cout << " Traveled graph has " << graph_data.tv << " vertices, " << graph_data.te << " edges." << std::endl;

	// write graph vector entries to disk
      #ifdef threading_enabled
	thr[0] = thread(MasterTmgThread, &graph_data, &list_mtx, &term_mtx, &all_waypoints, &et);
	// start at t=1, because MasterTmgThread will spawn another SubgraphThread when finished
	for (unsigned int t = 1; t < thr.size(); t++)
	  thr[t] = thread(SubgraphThread, t, &list_mtx, &term_mtx, &graph_data, &all_waypoints, &et);
	THREADLOOP thr[t].join();
      #else
	for (	graph_data.write_master_graphs_tmg();
		GraphListEntry::num < GraphListEntry::entries.size();
		GraphListEntry::num += 3
	    )	graph_data.write_subgraphs_tmg(GraphListEntry::num, 0, &all_waypoints, &et, &term_mtx);
      #endif
	cout << '!' << endl;
} //*/

cout << et.et() << "Clearing HighwayGraph contents from memory." << endl;
