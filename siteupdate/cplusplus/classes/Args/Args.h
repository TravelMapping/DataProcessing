#include <iostream>
#include <list>

class Args
{	public:
	/* w */ static std::string highwaydatapath;
	/* s */ static std::string systemsfile;
	/* u */ static std::string userlistfilepath;
	/* d */ static std::string databasename;
	/* l */ static std::string logfilepath;
	/* c */ static std::string csvstatfilepath;
	/* g */ static std::string graphfilepath;
	/* k */ static bool skipgraphs;
	/* n */ static std::string nmpmergepath;
	/* p */ static std::string splitregion, splitregionpath;
	/* U */ static std::list<std::string> userlist;
	/* t */ static int numthreads;
	/* e */ static bool errorcheck;
	/* T */ static int timeprecision;
	/* v */ static bool mtvertices;
	/* C */ static bool mtcsvfiles;
		static const char* exec;

	static bool init(int argc, char *argv[]);
	static void show_help();
};
