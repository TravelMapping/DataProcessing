#include "ElapsedTime.h"

ElapsedTime::ElapsedTime()
{	start_time = std::chrono::steady_clock::now();
}

std::string ElapsedTime::et()
{	using namespace std::chrono;
	duration<double> elapsed = duration_cast<duration<double>>(steady_clock::now() - start_time);
	char str[16];
	sprintf(str, "[%.1f] ", elapsed.count());
	return str;
}
