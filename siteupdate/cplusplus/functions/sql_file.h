class ElapsedTime;
#include <array>
#include <list>
#include <mutex>
#include <vector>

void sqlfile1
(	ElapsedTime*,
	std::vector<std::pair<std::string,std::string>>*,
	std::vector<std::pair<std::string,std::string>>*,
	std::list<std::string*>*,
	std::list<std::string*>*,
	std::mutex*
);

void sqlfile2(ElapsedTime*, std::list<std::array<std::string,3>>*);
