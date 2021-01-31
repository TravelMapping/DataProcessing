class Arguments;
class ClinchedDBValues;
class ElapsedTime;
class GraphListEntry;
class HighwaySystem;
class Region;
class TravelerList;
#include <array>
#include <iostream>
#include <list>
#include <vector>

void sqlfile1
(	ElapsedTime*,
	Arguments*,
	std::vector<Region*>*,
	std::vector<std::pair<std::string,std::string>>*,
	std::vector<std::pair<std::string,std::string>>*,
	std::list<HighwaySystem*>*,
	std::list<TravelerList*>*,
	ClinchedDBValues*,
	std::list<std::string*>*,
	std::list<std::string*>*
);

void sqlfile2(ElapsedTime*, Arguments*, std::list<std::array<std::string,3>>*, std::vector<GraphListEntry>*);
