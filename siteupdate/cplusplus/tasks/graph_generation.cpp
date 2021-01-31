// Build a graph structure out of all highway data in active and
// preview systems
cout << et.et() << "Setting up for graphs of highway data." << endl;
HighwayGraph graph_data(all_waypoints, highway_systems, args.numthreads, et);

cout << et.et() << "Writing graph waypoint simplification log." << endl;
ofstream wslogfile(args.logfilepath + "/waypointsimplification.log");
for (string& line : graph_data.waypoint_naming_log)
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
	graph_types.push_back({"master", "All Travel Mapping Data", "These graphs contain all routes currently plotted in the Travel Mapping project."});
	size_t graphnum = 3;

	cout << et.et() << "Writing master TM graph files." << endl;
	graph_vector[0].edges = 0;	graph_vector[0].vertices = graph_data.vertices.size();
	graph_vector[1].edges = 0;	graph_vector[1].vertices = 0;
	graph_vector[2].edges = 0;	graph_vector[2].vertices = 0;
	// get master graph vertex & edge counts for terminal output before writing files
	for (std::pair<Waypoint* const, HGVertex*>& wv : graph_data.vertices)
	{	graph_vector[0].edges += wv.second->incident_s_edges.size();
		if (wv.second->visibility >= 1)
		{	graph_vector[2].vertices++;
			graph_vector[2].edges += wv.second->incident_t_edges.size();
			if (wv.second->visibility == 2)
			{	graph_vector[1].vertices++;
				graph_vector[1].edges += wv.second->incident_c_edges.size();
			}
		}
	}
	graph_vector[0].edges /= 2;
	graph_vector[1].edges /= 2;
	graph_vector[2].edges /= 2;
	// print summary info
	std::cout << "   Simple graph has " << graph_vector[0].vertices << " vertices, " << graph_vector[0].edges << " edges." << std::endl;
	std::cout << "Collapsed graph has " << graph_vector[1].vertices << " vertices, " << graph_vector[1].edges << " edges." << std::endl;
	std::cout << " Traveled graph has " << graph_vector[2].vertices << " vertices, " << graph_vector[2].edges << " edges." << std::endl;

      #ifndef threading_enabled
	graph_data.write_master_graphs_tmg(graph_vector, args.graphfilepath + "/", traveler_lists);
      #else
	cout << et.et() << "Setting up subgraphs." << endl;
      #endif
	#include "subgraphs/continent.cpp"
	#include "subgraphs/multisystem.cpp"
	#include "subgraphs/system.cpp"
	#include "subgraphs/country.cpp"
	#include "subgraphs/multiregion.cpp"
	#include "subgraphs/area.cpp"
	#include "subgraphs/region.cpp"
      #ifdef threading_enabled
	// write graph_vector entries to disk
	thr[0] = new thread(MasterTmgThread, &graph_data, &graph_vector, args.graphfilepath+'/', &traveler_lists, &graphnum, &list_mtx, &term_mtx, &all_waypoints, &et);
	// set up for threaded subgraph generation
	// start at t=1, because MasterTmgThread will spawn another SubgraphThread when finished
	for (unsigned int t = 1; t < args.numthreads; t++)
		thr[t] = new thread(SubgraphThread, t, &graph_data, &graph_vector, &graphnum, &list_mtx, &term_mtx, args.graphfilepath+'/', &all_waypoints, &et);
	for (unsigned int t = 0; t < args.numthreads; t++)
		thr[t]->join();
	for (unsigned int t = 0; t < args.numthreads; t++)
		delete thr[t];
	cout << '!' << endl;
      #endif
     }

graph_data.clear();
