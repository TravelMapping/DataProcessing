#include "ElapsedTime.h"

ElapsedTime::ElapsedTime(int precision)
{	start_time = std::chrono::steady_clock::now();
	format = "[%.1f] ";
	format[3] = '0' + precision;
	str = new char[15+precision];
}

std::string ElapsedTime::et()
{	using namespace std::chrono;
	duration<double> elapsed = duration_cast<duration<double>>(steady_clock::now() - start_time);
	sprintf(str, format.data(), elapsed.count());
	return str;
}
