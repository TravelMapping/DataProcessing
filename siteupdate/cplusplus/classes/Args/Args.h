#include <iostream>
#include <list>

class Args
{	public:
	/* w */ static std::string datapath;
	/* s */ static std::string systemsfile;
	/* u */ static std::string userlistfilepath;
	/* x */ static std::string userlistext;
	/* d */ static std::string databasename;
	/* D */ static std::string databasepath;
	/* l */ static std::string logfilepath;
	/* c */ static std::string csvstatfilepath;
	/* g */ static std::string graphfilepath;
	/* k */ static bool skipgraphs;
	/* n */ static std::string nmpmergepath;
	/* p */ static std::string splitregion, splitregionpath, splitregionapp;
	/* U */ static std::list<std::string> userlist;
	/* t */ static int numthreads;
	/* e */ static bool errorcheck;
	/* T */ static int timeprecision;
	/* v */ static bool mtvertices;
	/* C */ static bool stcsvfiles;
	/* E */ static bool edgecounts;
	/* b */ static bool bitsetlogs;
	/* L */ static int colocationlimit;
	/* N */ static double nmpthreshold; 
		static const char* exec;

	static bool init(int argc, char *argv[]);
	static void show_help();
};
