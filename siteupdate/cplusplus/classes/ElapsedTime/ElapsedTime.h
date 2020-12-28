#include <chrono>
#include <string>

class ElapsedTime
{	//To get a nicely-formatted elapsed time string for printing
	std::chrono::steady_clock::time_point start_time;

	public:
	ElapsedTime();
	std::string et();
};
