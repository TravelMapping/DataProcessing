#include <iostream>
#include <list>

class Arguments
{	public:
	/* w */ std::string highwaydatapath;
	/* s */ std::string systemsfile;
	/* u */ std::string userlistfilepath;
	/* d */ std::string databasename;
	/* l */ std::string logfilepath;
	/* c */ std::string csvstatfilepath;
	/* g */ std::string graphfilepath;
	/* k */ bool skipgraphs;
	/* n */ std::string nmpmergepath;
	/* p */ std::string splitregion, splitregionpath;
	/* U */ std::list<std::string> userlist;
	/* t */ int numthreads;
	/* e */ bool errorcheck;
	/* T */ int timeprecision;
	/* h */ bool help;
		const char* exec;

	Arguments(int argc, char *argv[]);

	void show_help();
};
