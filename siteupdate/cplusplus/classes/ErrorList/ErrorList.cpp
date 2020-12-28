#include "ErrorList.h"

void ErrorList::add_error(std::string e)
{	mtx.lock();
	std::cout << "ERROR: " << e << std::endl;
	error_list.push_back(e);
	mtx.unlock();
}
