#include "Args.h"
#include <cstring>

/* t */ int Args::numthreads = 4;
/* T */ int Args::timeprecision = 1;
/* e */ bool Args::errorcheck = 0;
/* k */ bool Args::skipgraphs = 0;
/* v */ bool Args::mtvertices = 0;
/* C */ bool Args::stcsvfiles = 0;
/* E */ bool Args::edgecounts = 0;
/* b */ bool Args::bitsetlogs = 0;
/* w */ std::string Args::datapath = "../../HighwayData";
/* s */ std::string Args::systemsfile = "systems.csv";
/* u */ std::string Args::userlistfilepath = "../../UserData/list_files";
/* x */ std::string Args::userlistext = ".list";
/* d */ std::string Args::databasename = "TravelMapping";
/* l */ std::string Args::logfilepath = ".";
/* c */ std::string Args::csvstatfilepath = ".";
/* g */ std::string Args::graphfilepath = ".";
/* n */ std::string Args::nmpmergepath = "";
/* p */ std::string Args::splitregionpath = "";
/* p */ std::string Args::splitregion, Args::splitregionapp;
/* U */ std::list<std::string> Args::userlist;
/* L */ int Args::colocationlimit = 0; /* disabled by default */
/* N */ double Args::nmpthreshold = 0.0005;
const char* Args::exec;

bool Args::init(int argc, char *argv[])
{	const char* slash = strrchr (argv[0], '/');
	exec = slash ? slash+1 : argv[0];

	// parsing
	for (unsigned int n = 1; n < argc; n++)
	#define ARG(N,S,L) ( n+N < argc && (!strcmp(argv[n],S) || !strcmp(argv[n],L)) )
	{	     if ARG(0, "-e", "--errorcheck")		 errorcheck = 1;
		else if ARG(0, "-k", "--skipgraphs")		 skipgraphs = 1;
		else if ARG(0, "-v", "--mt-vertices")		 mtvertices = 1;
		else if ARG(0, "-C", "--st-csvs")		 stcsvfiles = 1;
		else if ARG(0, "-E", "--edge-counts")		 edgecounts = 1;
		else if ARG(0, "-b", "--bitset-logs")		 bitsetlogs = 1;
		else if ARG(0, "-h", "--help")			{show_help(); return 1;}
		else if ARG(1, "-w", "--datapath")		{datapath	  = argv[++n];}
		else if ARG(1, "-s", "--systemsfile")		{systemsfile      = argv[++n];}
		else if ARG(1, "-u", "--userlistfilepath")	{userlistfilepath = argv[++n];}
		else if ARG(1, "-x", "--userlistext")		{userlistext	  = argv[++n];}
		else if ARG(1, "-d", "--databasename")		{databasename     = argv[++n];}
		else if ARG(1, "-l", "--logfilepath")		{logfilepath      = argv[++n];}
		else if ARG(1, "-c", "--csvstatfilepath")	{csvstatfilepath  = argv[++n];}
		else if ARG(1, "-g", "--graphfilepath")		{graphfilepath    = argv[++n];}
		else if ARG(1, "-n", "--nmpmergepath")		{nmpmergepath     = argv[++n];}
		else if ARG(1, "-L", "--colocationlimit")
		{	colocationlimit = strtol(argv[++n], 0, 10);
			if (colocationlimit<0) colocationlimit=0;
		}
		else if ARG(1, "-t", "--numthreads")
		{	numthreads = strtol(argv[++n], 0, 10);
			if (numthreads<1) numthreads=1;
		}
		else if ARG(1, "-T", "--timeprecision")
		{	timeprecision = strtol(argv[++n], 0, 10);
			if (timeprecision<1) timeprecision=1;
			if (timeprecision>9) timeprecision=9;
		}
		else if ARG(1, "-N", "--nmp-threshold")
		{	nmpthreshold = strtod(argv[++n], 0);
			if (nmpthreshold<0) nmpthreshold=0.0005;  /* default */
		}
		else if ARG(1, "-U", "--userlist")
			while (n+1 < argc && argv[n+1][0] != '-')
			{	userlist.push_back(argv[n+1]);
				n++;
			}
		else if ARG(3, "-p", "--splitregion")
		{	splitregionpath = argv[++n];
			splitregionapp = argv[++n];
			splitregion = argv[++n];
		}

		if (userlistext.size() < 2)
		{	std::cout << "Fatal error: user list file extension must be at least 2 characters.\n";
			return 1;
		}
	}
	#undef ARG
	return 0;
}

void Args::show_help()
{	std::string indent(strlen(exec), ' ');
	std::cout  <<  "usage: " << exec << " [-h] [-w DATAPATH] [-s SYSTEMSFILE]\n";
	std::cout  <<  indent << "        [-u USERLISTFILEPATH] [-x USERLISTEXT] [-d DATABASENAME] [-l LOGFILEPATH]\n";
	std::cout  <<  indent << "        [-c CSVSTATFILEPATH] [-g GRAPHFILEPATH] [-k]\n";
	std::cout  <<  indent << "        [-n NMPMERGEPATH] [-p SPLITREGIONPATH SPLITREGION]\n";
	std::cout  <<  indent << "        [-U USERLIST [USERLIST ...]] [-t NUMTHREADS] [-e]\n";
	std::cout  <<  indent << "        [-T TIMEPRECISION] [-v] [-C] [-E] [-b]\n";
	std::cout  <<  indent << "        [-L COLOCATIONLIMIT] [-N NMPTHRESHOLD]\n";
	std::cout  <<  "\n";
	std::cout  <<  "Create SQL, stats, graphs, and log files from highway and user data for the\n";
	std::cout  <<  "Travel Mapping project.\n";
	std::cout  <<  "\n";
	std::cout  <<  "optional arguments:\n";
	std::cout  <<  "  -h, --help            show this help message and exit\n";
	std::cout  <<  "  -w DATAPATH, --datapath DATAPATH\n";
	std::cout  <<  "		        path to the route data repository\n";
	std::cout  <<  "  -s SYSTEMSFILE, --systemsfile SYSTEMSFILE\n";
	std::cout  <<  "		        file of highway systems to include\n";
	std::cout  <<  "  -u USERLISTFILEPATH, --userlistfilepath USERLISTFILEPATH\n";
	std::cout  <<  "		        path to the user list file data\n";
	std::cout  <<  "  -x USERLISTEXT, --userlistext USERLISTEXT\n";
	std::cout  <<  "		        file extension for user list files\n";
	std::cout  <<  "  -d DATABASENAME, --databasename DATABASENAME\n";
	std::cout  <<  "		        Database name for .sql file name\n";
	std::cout  <<  "  -l LOGFILEPATH, --logfilepath LOGFILEPATH\n";
	std::cout  <<  "		        Path to write log files, which should have a \"users\"\n";
	std::cout  <<  "		        subdirectory\n";
	std::cout  <<  "  -c CSVSTATFILEPATH, --csvstatfilepath CSVSTATFILEPATH\n";
	std::cout  <<  "		        Path to write csv statistics files\n";
	std::cout  <<  "  -g GRAPHFILEPATH, --graphfilepath GRAPHFILEPATH\n";
	std::cout  <<  "		        Path to write graph format data files\n";
	std::cout  <<  "  -k, --skipgraphs      Turn off generation of graph files\n";
	std::cout  <<  "  -n NMPMERGEPATH, --nmpmergepath NMPMERGEPATH\n";
	std::cout  <<  "		        Path to write data with NMPs merged (generated only if\n";
	std::cout  <<  "		        specified)\n";
	std::cout  <<  "  -p PATH SUFFIX REGION, --splitregion PATH SUFFIX REGION\n";
	std::cout  <<  "		        Path to logs & .lists for a specific\n";
	std::cout  <<  "		        region being split into subregions,\n";
	std::cout  <<  "		        and suffix to append to its systems.\n";
	std::cout  <<  "		        For Development.\n";
	std::cout  <<  "  -U USERLIST [USERLIST ...], --userlist USERLIST [USERLIST ...]\n";
	std::cout  <<  "		        For Development: list of users to use in dataset\n";
	std::cout  <<  "  -t NUMTHREADS, --numthreads NUMTHREADS\n";
	std::cout  <<  "		        Number of threads to use for concurrent tasks\n";
	std::cout  <<  "  -e, --errorcheck      Run only the subset of the process needed to verify\n";
	std::cout  <<  "		        highway data changes\n";
	std::cout  <<  "  -T TIMEPRECISION, --timeprecision TIMEPRECISION\n";
	std::cout  <<  "		        Number of digits (1-9) after decimal point in\n";
	std::cout  <<  "		        timestamp readouts\n";
	std::cout  <<  "  -v, --mt-vertices     Multi-threaded vertex construction\n";
	std::cout  <<  "  -C, --st-csvs         Single-threaded stats csv files\n";
	std::cout  <<  "  -E, --edge-counts     Report the quantity of each format graph edge\n";
	std::cout  <<  "  -b, --bitset-logs     Write TMBitset RAM use logs for region & system\n";
	std::cout  <<  "		        vertices & edges\n";
	std::cout  <<  "  -L, --colocationlimit COLOCATIONLIMIT\n";
	std::cout  <<  "		        Threshold to report colocation counts\n";
	std::cout  <<  "  -N, --nmp-threshold NMPTHRESHOLD\n";
	std::cout  <<  "		        Threshold to report near-miss points\n";
}
