// Build a graph structure out of all highway data in active and
// preview systems
cout << et.et() << "Setting up for graphs of highway data." << endl;
HighwayGraph graph_data(all_waypoints, et);

cout << et.et() << "Writing graph waypoint simplification log." << endl;
ofstream wslogfile(Args::logfilepath + "/waypointsimplification.log");
for (string& line : graph_data.waypoint_naming_log)
	wslogfile << line << '\n';
wslogfile.close();
graph_data.waypoint_naming_log.clear();

// create list of graph information for the DB
list<array<string,3>> graph_types;

// start generating graphs and making entries for graph DB table
if (Args::skipgraphs || Args::errorcheck)
	cout << et.et() << "SKIPPING generation of subgraphs." << endl;
else {	list<Region*> *regions;
	list<HighwaySystem*> *systems;
	GraphListEntry::entries.emplace_back('s', graph_data.vertices.size(), graph_data.se, 0);
	GraphListEntry::entries.emplace_back('c', graph_data.cv, graph_data.ce, 0);
	GraphListEntry::entries.emplace_back('t', graph_data.tv, graph_data.te, TravelerList::allusers.size);
	graph_types.push_back({"master", "All Travel Mapping Data",
				"These graphs contain all routes currently plotted in the Travel Mapping project."});
	GraphListEntry::num = 3;

	cout << et.et() << "Writing master TM graph files." << endl;
	// print summary info
	std::cout << "   Simple graph has " << graph_data.vertices.size() << " vertices, " << graph_data.se << " edges." << std::endl;
	std::cout << "Collapsed graph has " << graph_data.cv << " vertices, " << graph_data.ce << " edges." << std::endl;
	std::cout << " Traveled graph has " << graph_data.tv << " vertices, " << graph_data.te << " edges." << std::endl;

      #ifndef threading_enabled
	graph_data.write_master_graphs_tmg();
      #else
	cout << et.et() << "Setting up subgraphs." << endl;
      #endif
	HighwaySystem *h, *const sys_end = HighwaySystem::syslist.end();
	#include "subgraphs/continent.cpp"
	#include "subgraphs/multisystem.cpp"
	#include "subgraphs/system.cpp"
	#include "subgraphs/country.cpp"
	#include "subgraphs/multiregion.cpp"
	#include "subgraphs/fullcustom.cpp"
	#include "subgraphs/region.cpp"
	#include "subgraphs/area.cpp"
      #ifdef threading_enabled
	// write graph vector entries to disk
	thr[0] = thread(MasterTmgThread, &graph_data, &list_mtx, &term_mtx, &all_waypoints, &et);
	// start at t=1, because MasterTmgThread will spawn another SubgraphThread when finished
	for (unsigned int t = 1; t < thr.size(); t++)
	  thr[t] = thread(SubgraphThread, t, &list_mtx, &term_mtx, &graph_data, &all_waypoints, &et);
	THREADLOOP thr[t].join();
	cout << '!' << endl;
      #endif
     }

cout << et.et() << "Clearing HighwayGraph contents from memory." << endl;
graph_data.clear();
