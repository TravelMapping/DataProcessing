#define FMT_HEADER_ONLY
#include "ElapsedTime.h"
#include <fmt/format.h>

ElapsedTime::ElapsedTime(int precision)
{	start_time = std::chrono::steady_clock::now();
	format = "[{:.1f}] ";
	format[4] = '0' + precision;
}

std::string ElapsedTime::et()
{	using namespace std::chrono;
	duration<double> elapsed = duration_cast<duration<double>>(steady_clock::now() - start_time);
	return fmt::format(format.data(), elapsed.count());
}
