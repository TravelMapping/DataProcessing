#define ADDGRAPH(F) GraphListEntry::entries.emplace_back(\
	"tm-master", "All Travel Mapping Data", F, 'M', (list<Region*>*)0, (list<HighwaySystem*>*)0, (PlaceRadius*)0)
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
	ADDGRAPH('s');
	ADDGRAPH('c');
	ADDGRAPH('t');
	#undef ADDGRAPH
	graph_types.push_back({"master", "All Travel Mapping Data",
				"These graphs contain all routes currently plotted in the Travel Mapping project."});
	GraphListEntry::num = 3;

	cout << et.et() << "Writing master TM graph files." << endl;
	#define GRAPH(G) GraphListEntry::entries[G]
	GRAPH(0).edges = 0;	GRAPH(0).vertices = graph_data.vertices.size();
	GRAPH(1).edges = 0;	GRAPH(1).vertices = 0;
	GRAPH(2).edges = 0;	GRAPH(2).vertices = 0;
	// get master graph vertex & edge counts for terminal output before writing files
	for (HGVertex* v : graph_data.vertices)
	{	GRAPH(0).edges += v->incident_s_edges.size();
		if (v->visibility >= 1)
		{	GRAPH(2).vertices++;
			GRAPH(2).edges += v->incident_t_edges.size();
			if (v->visibility == 2)
			{	GRAPH(1).vertices++;
				GRAPH(1).edges += v->incident_c_edges.size();
			}
		}
	}
	GRAPH(0).edges /= 2;
	GRAPH(1).edges /= 2;
	GRAPH(2).edges /= 2;
	// print summary info
	std::cout << "   Simple graph has " << GRAPH(0).vertices << " vertices, " << GRAPH(0).edges << " edges." << std::endl;
	std::cout << "Collapsed graph has " << GRAPH(1).vertices << " vertices, " << GRAPH(1).edges << " edges." << std::endl;
	std::cout << " Traveled graph has " << GRAPH(2).vertices << " vertices, " << GRAPH(2).edges << " edges." << std::endl;
	#undef GRAPH

      #ifndef threading_enabled
	graph_data.write_master_graphs_tmg();
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
	// write graph vector entries to disk
	thr[0] = thread(MasterTmgThread, &graph_data, &list_mtx, &term_mtx, &all_waypoints, &et);
	// start at t=1, because MasterTmgThread will spawn another SubgraphThread when finished
	for (unsigned int t = 1; t < thr.size(); t++)
	  thr[t] = thread(SubgraphThread, t, &list_mtx, &term_mtx, &graph_data, &all_waypoints, &et);
	THREADLOOP thr[t].join();
	cout << '!' << endl;
      #endif
     }

graph_data.clear();
