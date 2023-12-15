list<Region*> *regions;
list<HighwaySystem*> *systems;
list<array<string,3>> graph_types; // create list of graph information for the DB
graph_types.push_back({"master", "All Travel Mapping Data",
			"These graphs contain all routes currently plotted in the Travel Mapping project."});
GraphListEntry::add_group("tm-master", "All Travel Mapping Data", 'M', nullptr, nullptr, nullptr);
HighwaySystem *h, *const sys_end = HighwaySystem::syslist.end();
#include "subgraphs/continent.cpp"
#include "subgraphs/multisystem.cpp"
#include "subgraphs/system.cpp"
#include "subgraphs/country.cpp"
#include "subgraphs/multiregion.cpp"
#include "subgraphs/fullcustom.cpp"
#include "subgraphs/region.cpp"
#include "subgraphs/area.cpp"
