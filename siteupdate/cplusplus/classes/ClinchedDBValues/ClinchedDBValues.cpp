#include "ClinchedDBValues.h"

void ClinchedDBValues::add_csmbr(std::string str)
{	csmbr_mtx.lock();
	csmbr_values.push_back(str);
	csmbr_mtx.unlock();
}

void ClinchedDBValues::add_ccr(std::string str)
{	ccr_mtx.lock();
	ccr_values.push_back(str);
	ccr_mtx.unlock();
}

void ClinchedDBValues::add_cr(std::string str)
{	cr_mtx.lock();
	cr_values.push_back(str);
	cr_mtx.unlock();
}
