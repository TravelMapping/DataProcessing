#include <chrono>
#include <string>

class ElapsedTime
{	//To get a nicely-formatted elapsed time string for printing
	std::chrono::steady_clock::time_point start_time;
	std::string format;
	char* str;

	public:
	ElapsedTime(int);
	std::string et();
};
