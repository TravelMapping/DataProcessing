#include "Args.h"
#include <cstring>

/* t */ int Args::numthreads = 4;
/* T */ int Args::timeprecision = 1;
/* e */ bool Args::errorcheck = 0;
/* k */ bool Args::skipgraphs = 0;
/* v */ bool Args::mtvertices = 0;
/* C */ bool Args::mtcsvfiles = 0;
/* w */ std::string Args::highwaydatapath = "../../../HighwayData";
/* s */ std::string Args::systemsfile = "systems.csv";
/* u */ std::string Args::userlistfilepath = "../../../UserData/list_files";
/* d */ std::string Args::databasename = "TravelMapping";
/* l */ std::string Args::logfilepath = ".";
/* c */ std::string Args::csvstatfilepath = ".";
/* g */ std::string Args::graphfilepath = ".";
/* n */ std::string Args::nmpmergepath = "";
/* p */ std::string Args::splitregionpath = "";
/* p */ std::string Args::splitregion;
/* U */ std::list<std::string> Args::userlist;
const char* Args::exec;

bool Args::init(int argc, char *argv[])
{	const char* slash = strrchr (argv[0], '/');
	exec = slash ? slash+1 : argv[0];

	// parsing
	for (unsigned int n = 1; n < argc; n++)
	#define ARG(N,S,L) ( n+N < argc && !strcmp(argv[n],S) || !strcmp(argv[n],L) )
	{	     if ARG(0, "-e", "--errorcheck")		 errorcheck = 1;
		else if ARG(0, "-k", "--skipgraphs")		 skipgraphs = 1;
		else if ARG(0, "-v", "--mt-vertices")		 mtvertices = 1;
		else if ARG(0, "-C", "--mt-csvs")		 mtcsvfiles = 1;
		else if ARG(0, "-h", "--help")			{show_help(); return 1;}
		else if ARG(1, "-w", "--highwaydatapath")	{highwaydatapath  = argv[n+1]; n++;}
		else if ARG(1, "-s", "--systemsfile")		{systemsfile      = argv[n+1]; n++;}
		else if ARG(1, "-u", "--userlistfilepath")	{userlistfilepath = argv[n+1]; n++;}
		else if ARG(1, "-d", "--databasename")		{databasename     = argv[n+1]; n++;}
		else if ARG(1, "-l", "--logfilepath")		{logfilepath      = argv[n+1]; n++;}
		else if ARG(1, "-c", "--csvstatfilepath")	{csvstatfilepath  = argv[n+1]; n++;}
		else if ARG(1, "-g", "--graphfilepath")		{graphfilepath    = argv[n+1]; n++;}
		else if ARG(1, "-n", "--nmpmergepath")		{nmpmergepath     = argv[n+1]; n++;}
		else if ARG(1, "-t", "--numthreads")
		{	numthreads = strtol(argv[n+1], 0, 10);
			if (numthreads<1) numthreads=1;
			n++;
		}
		else if ARG(1, "-T", "--timeprecision")
		{	timeprecision = strtol(argv[n+1], 0, 10);
			if (timeprecision<1) timeprecision=1;
			if (timeprecision>9) timeprecision=9;
			n++;
		}
		else if ARG(1, "-U", "--userlist")
			while (n+1 < argc && argv[n+1][0] != '-')
			{	userlist.push_back(argv[n+1]);
				n++;
			}
		else if ARG(2, "-p", "--splitregion")
		{	splitregionpath = argv[n+1];
			splitregion = argv[n+2];
			n += 2;
		}
	}
	#undef ARG
	return 0;
}

void Args::show_help()
{	std::string indent(strlen(exec), ' ');
	std::cout  <<  "usage: " << exec << " [-h] [-w HIGHWAYDATAPATH] [-s SYSTEMSFILE]\n";
	std::cout  <<  indent << "        [-u USERLISTFILEPATH] [-d DATABASENAME] [-l LOGFILEPATH]\n";
	std::cout  <<  indent << "        [-c CSVSTATFILEPATH] [-g GRAPHFILEPATH] [-k]\n";
	std::cout  <<  indent << "        [-n NMPMERGEPATH] [-p SPLITREGIONPATH SPLITREGION]\n";
	std::cout  <<  indent << "        [-U USERLIST [USERLIST ...]] [-t NUMTHREADS] [-e]\n";
	std::cout  <<  indent << "        [-T TIMEPRECISION] [-v]\n";
	std::cout  <<  "\n";
	std::cout  <<  "Create SQL, stats, graphs, and log files from highway and user data for the\n";
	std::cout  <<  "Travel Mapping project.\n";
	std::cout  <<  "\n";
	std::cout  <<  "optional arguments:\n";
	std::cout  <<  "  -h, --help            show this help message and exit\n";
	std::cout  <<  "  -w HIGHWAYDATAPATH, --highwaydatapath HIGHWAYDATAPATH\n";
	std::cout  <<  "		        path to the root of the highway data directory\n";
	std::cout  <<  "		        structure\n";
	std::cout  <<  "  -s SYSTEMSFILE, --systemsfile SYSTEMSFILE\n";
	std::cout  <<  "		        file of highway systems to include\n";
	std::cout  <<  "  -u USERLISTFILEPATH, --userlistfilepath USERLISTFILEPATH\n";
	std::cout  <<  "		        path to the user list file data\n";
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
	std::cout  <<  "  -p SPLITREGIONPATH SPLITREGION, --splitregion SPLITREGIONPATH SPLITREGION\n";
	std::cout  <<  "		        Path to logs & .lists for a specific...\n";
	std::cout  <<  "		        Region being split into subregions.\n";
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
	std::cout  <<  "  -C, --mt-csvs         Multi-threaded stats csv files\n";
}
