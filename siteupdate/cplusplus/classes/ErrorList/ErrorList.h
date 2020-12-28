#include <iostream>
#include <mutex>
#include <vector>

class ErrorList
{	/* Track a list of potentially fatal errors */
	std::mutex mtx;
	public:
	std::vector<std::string> error_list;
	void add_error(std::string);
};
