class ElapsedTime;
#include <array>
#include <list>
#include <mutex>

void sqlfile1
(	ElapsedTime*,
	std::list<std::string*>*,
	std::list<std::string*>*,
	std::mutex*
);

void sqlfile2(ElapsedTime*, std::list<std::array<std::string,3>>*);
