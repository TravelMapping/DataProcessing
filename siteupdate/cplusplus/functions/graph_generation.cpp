// Build a graph structure out of all highway data in active and
// preview systems
cout << et.et() << "Setting up for graphs of highway data." << endl;
HighwayGraph graph_data(all_waypoints, highway_systems, datacheckerrors, args.numthreads, et);

cout << et.et() << "Writing graph waypoint simplification log." << endl;
filename = args.logfilepath + "/waypointsimplification.log";
ofstream wslogfile(filename.data());
for (string line : graph_data.waypoint_naming_log)
	wslogfile << line << '\n';
wslogfile.close();
graph_data.waypoint_naming_log.clear();

// create list of graph information for the DB
vector<GraphListEntry> graph_vector;
list<array<string,3>> graph_types;

// start generating graphs and making entries for graph DB table

if (args.skipgraphs || args.errorcheck)
	cout << et.et() << "SKIPPING generation of subgraphs." << endl;
else {	list<Region*> *regions;
	list<HighwaySystem*> *systems;

	graph_vector.emplace_back("tm-master", "All Travel Mapping Data",
				  's', 'M', (list<Region*>*)0, (list<HighwaySystem*>*)0, (PlaceRadius*)0);
	graph_vector.emplace_back("tm-master", "All Travel Mapping Data",
				  'c', 'M', (list<Region*>*)0, (list<HighwaySystem*>*)0, (PlaceRadius*)0);
	graph_vector.emplace_back("tm-master", "All Travel Mapping Data",
				  't', 'M', (list<Region*>*)0, (list<HighwaySystem*>*)0, (PlaceRadius*)0);

      /*#ifdef threading_enabled
	if (args.numthreads <= 1)
      #endif//*/
	     {	cout << et.et() << "Writing master TM simple graph file, tm-master-simple.tmg" << endl;
		graph_data.write_master_tmg_simple(&graph_vector[0], args.graphfilepath+"/tm-master-simple.tmg");
		cout << et.et() << "Writing master TM collapsed graph file, tm-master-collapsed.tmg." << endl;
		graph_data.write_master_tmg_collapsed(&graph_vector[1], args.graphfilepath+"/tm-master-collapsed.tmg", 0);
		cout << et.et() << "Writing master TM traveled graph file, tm-master-traveled.tmg." << endl;
		graph_data.write_master_tmg_traveled(&graph_vector[2], args.graphfilepath+"/tm-master-traveled.tmg", &traveler_lists, 0);
	     }
      /*#ifdef threading_enabled
	else {	cout << et.et() << "Writing master TM simple graph file, tm-master-simple.tmg" << endl;
		thr[0] = new thread(MasterTmgSimpleThread, &graph_data, &graph_vector[0], args.graphfilepath+"/tm-master-simple.tmg");
		cout << et.et() << "Writing master TM collapsed graph file, tm-master-collapsed.tmg." << endl;
		thr[1] = new thread(MasterTmgCollapsedThread, &graph_data, &graph_vector[1], args.graphfilepath+"/tm-master-collapsed.tmg");
		thr[0]->join();
		thr[1]->join();
		delete thr[0];
		delete thr[1];
	     }
      #endif//*/

	graph_types.push_back({"master", "All Travel Mapping Data", "These graphs contain all routes currently plotted in the Travel Mapping project."});
	size_t graphnum = 3;
      #ifdef threading_enabled
	cout << et.et() << "Setting up subgraphs." << flush;
      #endif
	#include "subgraphs/continent.cpp"
	#include "subgraphs/multisystem.cpp"
	#include "subgraphs/system.cpp"
	#include "subgraphs/country.cpp"
	#include "subgraphs/multiregion.cpp"
	#include "subgraphs/area.cpp"
	#include "subgraphs/region.cpp"
      #ifdef threading_enabled
	// write new graph_vector entries to disk
	// set up for threaded subgraph generation
	for (unsigned int t = 0; t < args.numthreads; t++)
		thr[t] = new thread(SubgraphThread, t, &graph_data, &graph_vector, &graphnum, &list_mtx, args.graphfilepath + "/", &all_waypoints, &et);
	for (unsigned int t = 0; t < args.numthreads; t++)
		thr[t]->join();
	for (unsigned int t = 0; t < args.numthreads; t++)
		delete thr[t];
	cout << '!' << endl;
      #endif
     }

graph_data.clear();
