#include "DatacheckEntry.h"
#include "../Route/Route.h"

std::mutex DatacheckEntry::mtx;
std::list<DatacheckEntry> DatacheckEntry::errors;

void DatacheckEntry::add(Route *rte, std::string l1, std::string l2, std::string l3, std::string c, std::string i)
{	mtx.lock();
	errors.emplace_back(rte, l1, l2, l3, c, i);
	mtx.unlock();
}

DatacheckEntry::DatacheckEntry(Route *rte, std::string l1, std::string l2, std::string l3, std::string c, std::string i)
{	route = rte;
	label1 = l1;
	label2 = l2;
	label3 = l3;
	code = c;
	info = i;
	fp = 0;
}

bool DatacheckEntry::match_except_info(std::array<std::string, 6> &fpentry)
{	// Check if the fpentry from the csv file matches in all fields
	// except the info field
	if (fpentry[0] != route->root)	return 0;
	if (fpentry[1] != label1)	return 0;
	if (fpentry[2] != label2)	return 0;
	if (fpentry[3] != label3)	return 0;
	if (fpentry[4] != code)		return 0;
	return 1;
}

// Original "Python list" format unused. Using "CSV style" format instead.
std::string DatacheckEntry::str()
{	return route->root + ";" + label1 + ";" + label2 + ";" + label3 + ";" + code + ";" + info;
}

bool operator < (DatacheckEntry &a, DatacheckEntry &b)
{	return a.str() < b.str();
}
