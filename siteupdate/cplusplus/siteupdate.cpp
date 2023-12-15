// Tab Width = 8

// Travel Mapping Project, Jim Teresco and Eric Bryant, 2015-2023
/* Code to read .csv and .wpt files and prepare for
adding to the Travel Mapping Project database.

(c) 2015-2023, Jim Teresco and Eric Bryant
Original Python version by Jim Teresco, with contributions from Eric Bryant and the TravelMapping team
C++ translation by Eric Bryant

This module defines classes to represent the contents of a
.csv file that lists the highways within a system, and a
.wpt file that lists the waypoints for a given highway.
*/

#include "classes/Args/Args.h"
#include "classes/DBFieldLength/DBFieldLength.h"
#include "classes/Datacheck/Datacheck.h"
#include "classes/ElapsedTime/ElapsedTime.h"
#include "classes/ErrorList/ErrorList.h"
#include "classes/GraphGeneration/GraphListEntry.h"
#include "classes/GraphGeneration/HGVertex.h"
#include "classes/GraphGeneration/HighwayGraph.h"
#include "classes/GraphGeneration/PlaceRadius.h"
#include "classes/HighwaySegment/HighwaySegment.h"
#include "classes/HighwaySystem/HighwaySystem.h"
#include "classes/Region/Region.h"
#include "classes/Route/Route.h"
#include "classes/TravelerList/TravelerList.h"
#include "classes/Waypoint/Waypoint.h"
#include "classes/WaypointQuadtree/WaypointQuadtree.h"
#include "functions/crawl_rte_data.h"
#include "functions/rdstats.h"
#include "functions/route_and_label_logs.h"
#include "functions/tmstring.h"
#include "functions/sql_file.h"
#ifdef threading_enabled
#include <thread>
#include "threads/threads.h"
#endif
void allbyregionactiveonly(std::mutex*, double);
void allbyregionactivepreview(std::mutex*, double);
using namespace std;

int main(int argc, char *argv[])
{	ifstream file;
	string line;
	mutex list_mtx, term_mtx;
	ErrorList el;
	double active_only_miles = 0;
	double active_preview_miles = 0;

	// argument parsing
	if (Args::init(argc, argv)) return 1;
      #ifndef threading_enabled
	Args::numthreads = 1;
      #else
	std::vector<std::thread> thr(Args::numthreads);
	#define THREADLOOP for (unsigned int t = 0; t < thr.size(); t++)
      #endif

	// start a timer for including elapsed time reports in messages
	ElapsedTime et(Args::timeprecision);
	time_t timestamp = time(0);
	cout << "Start: " << ctime(&timestamp);

	// Get list of travelers in the system
	cout << et.et() << "Making list of travelers." << endl;
	TravelerList::get_ids(el);

	cout << et.et() << "Reading region, country, and continent descriptions." << endl;
	Region::read_csvs(el);

	// Create an array of HighwaySystem objects, one per system in systems.csv file
	cout << et.et() << "Reading systems list in " << Args::datapath << "/" << Args::systemsfile << "." /*<< endl*/;
	HighwaySystem::systems_csv(el);

	// For tracking whether any .wpt files are in the directory tree
	// that do not have a .csv file entry that causes them to be
	// read into the data
	cout << et.et() << "Finding all .wpt files. " << flush;
	unordered_set<string> splitsystems;
	crawl_rte_data(Args::datapath+"/data", splitsystems, 0);
	cout << Route::all_wpt_files.size() << " files found." << endl;

	// For finding colocated Waypoints and concurrent segments, we have
	// quadtree of all Waypoints in existence to find them efficiently
	WaypointQuadtree all_waypoints(-90,-180,90,180);

	cout << et.et() << "Reading waypoints for all routes." << endl;
	#include "tasks/threaded/ReadWpt.cpp"

	//cout << et.et() << "Writing WaypointQuadtree.tmg." << endl;
	//all_waypoints.write_qt_tmg(Args::logfilepath+"/WaypointQuadtree.tmg");
	cout << et.et() << "Sorting waypoints in Quadtree." << endl;
	all_waypoints.sort();

	cout << et.et() << "Finding unprocessed wpt files." << endl;
	if (Route::all_wpt_files.size())
	{	ofstream unprocessedfile(Args::logfilepath+"/unprocessedwpts.log");
		cout << Route::all_wpt_files.size() << " .wpt files in " << Args::datapath << "/data not processed, see unprocessedwpts.log." << endl;
		list<string> all_wpts_list(Route::all_wpt_files.begin(), Route::all_wpt_files.end());
		all_wpts_list.sort();
		for (const string &f : all_wpts_list) unprocessedfile << strstr(f.data(), "data") << '\n';
		unprocessedfile.close();
		Route::all_wpt_files.clear();
	}
	else	cout << "All .wpt files in " << Args::datapath << "/data processed." << endl;

      #ifdef threading_enabled
	cout << et.et() << "Searching for near-miss points." << endl;
	HighwaySystem::it = HighwaySystem::syslist.begin();
	THREADLOOP thr[t] = thread(NmpSearchThread, t, &list_mtx, &all_waypoints);
	THREADLOOP thr[t].join();
      #endif

	cout << et.et() << "Near-miss point log and tm-master.nmp file." << endl;
	all_waypoints.nmplogs();

	// if requested, rewrite data with near-miss points merged in
	if (Args::nmpmergepath != "" && !Args::errorcheck)
	{	cout << et.et() << "Writing near-miss point merged wpt files." << endl;
		#include "tasks/threaded/NmpMerged.cpp"
	}

	cout << et.et() << "Concurrent segment detection." << flush;
	#include "tasks/concurrency_detection.cpp"

	cout << et.et() << "Creating label hashes and checking route integrity." << endl;
	#include "tasks/threaded/RteInt.cpp"

	#include "tasks/read_updates.cpp"

	cout << et.et() << "Processing traveler list files:" << endl;
	#include "tasks/threaded/ReadList.cpp"
	cout << endl << et.et() << "Processed " << TravelerList::allusers.size << " traveler list files." << endl;

	cout << et.et() << "Clearing route & label hash tables." << endl;
	Route::root_hash.clear();
	Route::pri_list_hash.clear();
	Route::alt_list_hash.clear();
	for (HighwaySystem& h : HighwaySystem::syslist)
	  for (Route& r : h.routes)
	  {	r.pri_label_hash.clear();
		r.alt_label_hash.clear();
		r.duplicate_labels.clear();
	  }

	cout << et.et() << "Writing route and label logs." << endl;
	route_and_label_logs(&timestamp);

	cout << et.et() << "Augmenting travelers for detected concurrent segments." << flush;
	#include "tasks/threaded/ConcAug.cpp"

	/*ofstream sanetravfile(Args::logfilepath+"/concurrent_travelers_sanity_check.log");
	for (HighwaySystem& h : HighwaySystem::syslist)
	    for (Route& r : h.routes)
		for (HighwaySegment& s : r.segments)
		    sanetravfile << s.concurrent_travelers_sanity_check();
	sanetravfile.close(); //*/

	// compute lots of regional stats:
	// overall, active+preview, active only,
	// and per-system which falls into just one of these categories
	cout << et.et() << "Computing stats." << flush;
	#include "tasks/threaded/CompStats.cpp"

	cout << et.et() << "Writing routedatastats.log." << endl;
	rdstats(active_only_miles, active_preview_miles, &timestamp);

	cout << et.et() << "Creating per-traveler stats logs and augmenting data structure." << flush;
	#include "tasks/threaded/UserLog.cpp"

	cout << et.et() << "Writing stats csv files." << endl;
	#include "tasks/threaded/StatsCsv.cpp"

	cout << et.et() << "Reading datacheckfps.csv." << endl;
	Datacheck::read_fps(el);
	cout << et.et() << "Marking datacheck false positives." << flush;
	Datacheck::mark_fps(et);
	cout << et.et() << "Writing log of unmatched datacheck FP entries." << endl;
	Datacheck::unmatchedfps_log();
	cout << et.et() << "Writing datacheck.log" << endl;
	Datacheck::datacheck_log();

	cout << et.et() << "Reading subgraph descriptions and checking for errors." << endl;
	#include "tasks/graph_setup.cpp"

	// See if we have any errors that should be fatal to the site update process
	if (el.error_list.size())
	{	cout << et.et() << "ABORTING due to " << el.error_list.size() << " errors:" << endl;
		for (unsigned int i = 0; i < el.error_list.size(); i++)
			cout << i+1 << ": " << el.error_list[i] << endl;
		return 1;
	}

      #ifdef threading_enabled
	thread sqlthread;
	if   (!Args::errorcheck)
	{	std::cout << et.et() << "Start writing database file " << Args::databasename << ".sql.\n" << std::flush;
		sqlthread=thread(sqlfile1, &et, &updates, &systemupdates, &term_mtx);
	}
      #endif

	if (!Args::skipgraphs && !Args::errorcheck)
	{	// Build a graph structure out of all highway data in active and preview systems
		#include "tasks/graph_generation.cpp"
	} else	cout << et.et() << "SKIPPING generation of graphs." << endl;

	if   (Args::errorcheck)
		cout << et.et() << "SKIPPING database file." << endl;
	else {
	      #ifdef threading_enabled
		sqlthread.join();
		std::cout << et.et() << "Resume writing database file " << Args::databasename << ".sql.\n" << std::flush;
	      #else
		std::cout << et.et() << "Writing database file " << Args::databasename << ".sql.\n" << std::flush;
		sqlfile1(&et, &updates, &systemupdates, &term_mtx);
	      #endif
		sqlfile2(&et, &graph_types);
	     }

	// print some statistics
	cout << et.et() << "Processed " << HighwaySystem::syslist.size << " highway systems." << endl;
	unsigned int routes = 0;
	unsigned int points = 0;
	unsigned int segments = 0;
	for (HighwaySystem& h : HighwaySystem::syslist)
	{	routes += h.routes.size;
		for (Route& r : h.routes)
		{	points += r.points.size;
			segments += r.segments.size;
		}
	}
	cout << "Processed " << routes << " routes with a total of " << points << " points and " << segments << " segments." << endl;
	if (points != all_waypoints.size())
	  cout << "MISMATCH: all_waypoints contains " << all_waypoints.size() << " waypoints!" << endl;
	cout << et.et() << "WaypointQuadtree contains " << all_waypoints.total_nodes() << " total nodes." << endl;

	vector<unsigned int> colocate_counts(2,0);
	if (!Args::errorcheck)
	{	// compute colocation of waypoints stats
	        cout << et.et() << "Computing waypoint colocation stats";
	        if (Args::colocationlimit)
		  {   cout << ", reporting all with " << Args::colocationlimit << " or more colocations:" << endl;
		  }
		else
		  {   cout << "." << endl;
		  }
		all_waypoints.final_report(colocate_counts);
		cout << "Waypoint colocation counts:" << endl;
		unsigned int unique_locations = 0;
		for (unsigned int c = 1; c < colocate_counts.size(); c++)
		{	unique_locations += colocate_counts[c];
			printf("%6i are each occupied by %2i waypoints.\n", colocate_counts[c], c);
		}
		cout << "Unique locations: " << unique_locations << endl;
	} else	all_waypoints.final_report(colocate_counts);

	/* EDB
	cout << endl;
	unsigned int a_count = 0;
	unsigned int p_count = 0;
	unsigned int d_count = 0;
	unsigned other_count = 0;
	unsigned int total_rtes = 0;
	for (HighwaySystem& h : HighwaySystem::syslist)
	  for (Route* r : h.route_list)
	  {	total_rtes++;
		if (h.devel()) d_count++;
		else {	if (h.active()) a_count++;
			else if (h.preview()) p_count++;
			     else other_count++;
		     }
	  }
	cout << a_count+p_count << " clinchable routes:" << endl;
	cout << a_count << " in active systems, and" << endl;
	cout << p_count << " in preview systems." << endl;
	cout << d_count << " routes in devel systems." << endl;
	if (other_count) cout << other_count << " routes not designated active/preview/devel!" << endl;
	cout << total_rtes << " total routes." << endl;//*/

	if (Args::errorcheck)
	    cout << "\n!!! DATA CHECK SUCCESSFUL !!!\n" << endl;

	timestamp = time(0);
	cout << "Finish: " << ctime(&timestamp);
	cout << "Total run time: " << et.et() << endl;

}
