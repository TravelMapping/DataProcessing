class DatacheckEntryList
{	std::mutex mtx;
	public:
	std::list<DatacheckEntry> entries;
	void add(Route *rte, std::string l1, std::string l2, std::string l3, std::string c, std::string i)
	{	mtx.lock();
		entries.emplace_back(rte, l1, l2, l3, c, i);
		mtx.unlock();
	}
};
