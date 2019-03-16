class ClinchedDBValues
{	public:
	// this will be used to store DB entry lines...
	std::list<std::string> csmbr_values;	// for clinchedSystemMileageByRegion table
	std::vector<std::string> ccr_values;	// and similar for DB entry lines for clinchedConnectedRoutes table
	std::vector<std::string> cr_values;	// and clinchedRoutes table
	// ...to be added into the DB later in the program
	std::mutex csmbr_mtx, ccr_mtx, cr_mtx;

	void add_csmbr(std::string str)
	{	csmbr_mtx.lock();
		csmbr_values.push_back(str);
		csmbr_mtx.unlock();
	}

	void add_ccr(std::string str)
	{	ccr_mtx.lock();
		ccr_values.push_back(str);
		ccr_mtx.unlock();
	}

	void add_cr(std::string str)
	{	cr_mtx.lock();
		cr_values.push_back(str);
		cr_mtx.unlock();
	}
};
