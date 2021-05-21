class ClinchedDBValues;
class ElapsedTime;
class GraphListEntry;
class HighwaySystem;
class Region;
class TravelerList;
#include <array>
#include <iostream>
#include <list>
#include <mutex>
#include <vector>

void sqlfile1
(	ElapsedTime*,
	std::vector<std::pair<std::string,std::string>>*,
	std::vector<std::pair<std::string,std::string>>*,
	ClinchedDBValues*,
	std::list<std::string*>*,
	std::list<std::string*>*,
	std::mutex*
);

void sqlfile2(ElapsedTime*, std::list<std::array<std::string,3>>*);
