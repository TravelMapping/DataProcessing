class ErrorList
{	/* Track a list of potentially fatal errors */

	std::mutex mtx;
	public:
	std::vector<std::string> error_list;
	void add_error(std::string e)
	{	mtx.lock();
		std::cout << "ERROR: " << e << std::endl;
		error_list.push_back(e);
		mtx.unlock();
	}
};
