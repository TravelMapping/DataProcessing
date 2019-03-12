class ElapsedTime
{	//To get a nicely-formatted elapsed time string for printing
	std::chrono::steady_clock::time_point start_time;

	public:
	ElapsedTime()
	{	start_time = std::chrono::steady_clock::now();
	}

	std::string et()
	{	using namespace std::chrono;
		duration<double> elapsed = duration_cast<duration<double>>(steady_clock::now() - start_time);
		char str[16];
		sprintf(str, "[%.1f] ", elapsed.count());
		return str;
	}
};
