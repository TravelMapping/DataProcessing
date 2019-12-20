// Tab Width = 8

// Travel Mapping Project, Jim Teresco, 2015-2018
/* Code to read .csv and .wpt files and prepare for
adding to the Travel Mapping Project database.

(c) 2015-2019, Jim Teresco and Eric Bryant
Original Python version by Jim Teresco, with contributions from Eric Bryant and the TravelMapping team
C++ translation by Eric Bryant

This module defines classes to represent the contents of a
.csv file that lists the highways within a system, and a
.wpt file that lists the waypoints for a given highway.
*/

class ErrorList;
class Waypoint;
class HighwaySegment;
class Route;
class ConnectedRoute;
class HighwaySystem;
class TravelerList;
class Region;
class DatacheckEntryList;
class HighwayGraph;
class HGVertex;
class HGEdge;
#include <chrono>
#include <cmath>
#include <cstring>
#include <ctime>
#include <deque>
#include <dirent.h>
#include <forward_list>
#include <fstream>
#include <iostream>
#include <list>
#include <mutex>
//#include <regex>	// Unbearably slow. Equivalent functions will be hand-coded instead.
#include <sys/stat.h>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "enable_threading.cpp"
#include "functions/lower.cpp"
#include "functions/upper.cpp"
#include "functions/format_clinched_mi.cpp"
#include "functions/double_quotes.cpp"
#include "functions/crawl_hwy_data.cpp"
#include "functions/operators.cpp"
#include "functions/list_contains.cpp"
#include "classes/Arguments.cpp"
#include "classes/WaypointQuadtree/WaypointQuadtree.h"
#include "classes/Route/Route.h"
#include "classes/ElapsedTime.cpp"
#include "classes/ErrorList.cpp"
#include "classes/Region.cpp"
#include "classes/HighwaySystem.cpp"
#include "classes/DatacheckEntry.cpp"
#include "classes/DatacheckEntryList.cpp"
#include "classes/Waypoint/Waypoint.h"
#include "classes/Waypoint/Waypoint.cpp"
#include "classes/WaypointQuadtree/WaypointQuadtree.cpp"
#include "classes/HighwaySegment/HighwaySegment.h"
#include "classes/ClinchedDBValues.cpp"
#include "classes/Route/Route.cpp"
#include "classes/ConnectedRoute.cpp"
#include "classes/TravelerList/TravelerList.cpp"
#include "classes/HighwaySegment/HighwaySegment.cpp"
#include "classes/GraphGeneration/HGEdge.h"
#include "classes/GraphGeneration/HGVertex.cpp"
#include "classes/GraphGeneration/PlaceRadius.h"
#include "classes/GraphGeneration/GraphListEntry.cpp"
#include "classes/GraphGeneration/HighwayGraph.cpp"
#include "classes/GraphGeneration/HGEdge.cpp"
#include "classes/GraphGeneration/PlaceRadius.cpp"
#include "threads/ReadWptThread.cpp"
#include "threads/NmpMergedThread.cpp"
#include "threads/ReadListThread.cpp"
#include "threads/ConcAugThread.cpp"
#include "threads/ComputeStatsThread.cpp"
#include "threads/UserLogThread.cpp"
#include "threads/SubgraphThread.cpp"
#include "threads/MasterTmgThread.cpp"
#include "functions/sql_file.cpp"
using namespace std;

int main(int argc, char *argv[])
{	ifstream file;
	string line;
	mutex list_mtx, log_mtx, strtok_mtx;
	time_t timestamp;

	// start a timer for including elapsed time reports in messages
	ElapsedTime et;

	timestamp = time(0);
	cout << "Start: " << ctime(&timestamp);

	// create ErrorList
	ErrorList el;

	// argument parsing
	Arguments args(argc, argv);
	if (args.help) return 0;

	// Get list of travelers in the system
	list<string> traveler_ids = args.userlist;
	args.userlist.clear();
	if (traveler_ids.empty())
	{	DIR *dir;
		dirent *ent;
		if ((dir = opendir (args.userlistfilepath.data())) != NULL)
		{	while ((ent = readdir (dir)) != NULL)
			{	string trav = ent->d_name;
				if (trav.size() > 5 && trav.substr(trav.size()-5, string::npos) == ".list")
					traveler_ids.push_back(trav);
			}
			closedir(dir);
		}
		else {	cout << "Error opening directory " << args.userlistfilepath << ". (Not found?)" << endl;
			return 0;
		     }
	}
	else for (string &id : traveler_ids) id += ".list";

	// read region, country, continent descriptions
	cout << et.et() << "Reading region, country, and continent descriptions." << endl;

	// continents
	list<pair<string, string>> continents;
	file.open(args.highwaydatapath+"/continents.csv");
	if (!file) el.add_error("Could not open "+args.highwaydatapath+"/continents.csv");
	else {	getline(file, line); // ignore header line
		while(getline(file, line))
		{	if (line.back() == 0x0D) line.erase(line.end()-1);	// trim DOS newlines
			size_t delim = line.find(';');
			if (delim == string::npos)
			{	el.add_error("Could not parse continents.csv line: " + line);
				continue;
			}
			string code = line.substr(0,delim);
			string name = line.substr(delim+1);
			if (name.find(';') != string::npos)
			{	el.add_error("Could not parse continents.csv line: " + line);
				continue;
			}
			continents.push_back(pair<string, string>(code, name));
		}
	     }
	file.close();

	// countries
	list<pair<string, string>> countries;
	file.open(args.highwaydatapath+"/countries.csv");
	if (!file) el.add_error("Could not open "+args.highwaydatapath+"/countries.csv");
	else {	getline(file, line); // ignore header line
		while(getline(file, line))
		{	if (line.back() == 0x0D) line.erase(line.end()-1);	// trim DOS newlines
			size_t delim = line.find(';');
			if (delim == string::npos)
			{	el.add_error("Could not parse countries.csv line: " + line);
				continue;
			}
			string code = line.substr(0,delim);
			string name = line.substr(delim+1);
			if (name.find(';') != string::npos)
			{	el.add_error("Could not parse countries.csv line: " + line);
				continue;
			}
			countries.push_back(pair<string, string>(code, name));
		}
	     }
	file.close();

	//regions
	list<Region> all_regions;
	unordered_map<string, Region*> region_hash;
	file.open(args.highwaydatapath+"/regions.csv");
	if (!file) el.add_error("Could not open "+args.highwaydatapath+"/regions.csv");
	else {	getline(file, line); // ignore header line
		while(getline(file, line))
		{	Region rg(line, countries, continents, el);
			if (rg.is_valid()) all_regions.push_back(rg);
			region_hash[all_regions.back().code] = &all_regions.back();
		}
	     }
	file.close();

	// Create a list of HighwaySystem objects, one per system in systems.csv file
	list<HighwaySystem*> highway_systems;
	cout << et.et() << "Reading systems list in " << args.highwaydatapath+"/"+args.systemsfile << "." << endl;
	file.open(args.highwaydatapath+"/"+args.systemsfile);
	if (!file) el.add_error("Could not open "+args.highwaydatapath+"/"+args.systemsfile);
	else {	getline(file, line); // ignore header line
		list<string> ignoring;
		while(getline(file, line))
		{	if (line.back() == 0x0D) line.erase(line.end()-1);	// trim DOS newlines
			if (line[0] == '#')
			{	ignoring.push_back("Ignored comment in " + args.systemsfile + ": " + line);
				continue;
			}
			HighwaySystem *hs = new HighwaySystem(line, el, args.highwaydatapath+"/hwy_data/_systems", args.systemsfile, countries, region_hash);
					    // deleted on termination of program
			if (hs->is_valid()) highway_systems.push_back(hs);
			else delete hs;
			cout << hs->systemname << '.' << std::flush;
		}
		cout << endl;
		// at the end, print the lines ignored
		for (string l : ignoring) cout << l << endl;
		ignoring.clear();
	     }
	file.close();

	DatacheckEntryList *datacheckerrors = new DatacheckEntryList;
					      // deleted on termination of program

	// check for duplicate .list names
	// and duplicate root entries among Route and ConnectedRoute
	// data in all highway systems
    {	cout << et.et() << "Checking for duplicate list names in routes, roots in routes and connected routes." << endl;
	unordered_set<string> roots, list_names, duplicate_list_names;
	unordered_set<Route*> con_roots;

	for (HighwaySystem *h : highway_systems)
	{	for (Route r : h->route_list)
		{	if (roots.find(r.root) == roots.end()) roots.insert(r.root);
			else el.add_error("Duplicate root in route lists: " + r.root);
			string list_name = r.readable_name();
			// FIXME branch based on result of list_names.insert
			if (list_names.find(list_name) == list_names.end()) list_names.insert(list_name);
			else duplicate_list_names.insert(list_name);
		}
		for (ConnectedRoute cr : h->con_route_list)
		  for (size_t r = 0; r < cr.roots.size(); r++)
		    // FIXME branch based on result of list_names.insert
		    if (con_roots.find(cr.roots[r]) == con_roots.end()) con_roots.insert(cr.roots[r]);
		    else el.add_error("Duplicate root in con_route lists: " + cr.roots[r]->root);//*/
	}

	// Make sure every route was listed as a part of some connected route
	if (roots.size() == con_roots.size())
		cout << "Check passed: same number of routes as connected route roots. " << roots.size() << endl;
	else {	el.add_error("Check FAILED: " + to_string(roots.size()) + " routes != " + to_string(con_roots.size()) + " connected route roots.");
		// remove con_routes entries from roots
		for (Route *cr : con_roots)
		{	unordered_set<string>::iterator r = roots.find(cr->root);
			if (r != roots.end()) roots.erase(r); //FIXME erase by value
		}
		// there will be some leftovers, let's look up their routes to make
		// an error report entry (not worried about efficiency as there would
		// only be a few in reasonable cases)
		unsigned int num_found = 0;
		for (HighwaySystem *h : highway_systems)
		  for (Route r : h->route_list)
		    for (string lr : roots)
		      if (lr == r.root)
		      {	el.add_error("route " + lr + " not matched by any connected route root.");
			num_found++;
			break;
		      }
		cout << "Added " << num_found << " ROUTE_NOT_IN_CONNECTED error entries." << endl;
	     }

	// report any duplicate list names as errors
	if (duplicate_list_names.empty()) cout << "No duplicate list names found." << endl;
	else {	for (string d : duplicate_list_names) el.add_error("Duplicate list name: " + d);
		cout << "Added " << duplicate_list_names.size() << " DUPLICATE_LIST_NAME error entries." << endl;
	     }

	roots.clear();
	list_names.clear();
	duplicate_list_names.clear();
	con_roots.clear();
    }

	// For tracking whether any .wpt files are in the directory tree
	// that do not have a .csv file entry that causes them to be
	// read into the data
	cout << et.et() << "Finding all .wpt files. " << flush;
	unordered_set<string> all_wpt_files, splitsystems;
	crawl_hwy_data(args.highwaydatapath+"/hwy_data", all_wpt_files, splitsystems, args.splitregion, 0);
	cout << all_wpt_files.size() << " files found." << endl;

	// For finding colocated Waypoints and concurrent segments, we have
	// quadtree of all Waypoints in existence to find them efficiently
	WaypointQuadtree all_waypoints(-90,-180,90,180);

	// Next, read all of the .wpt files for each HighwaySystem
	cout << et.et() << "Reading waypoints for all routes." << endl;
      #ifdef threading_enabled
	// set up for threaded processing of highway systems
	list<HighwaySystem*>::iterator hs_it = highway_systems.begin();

	thread **thr = new thread*[args.numthreads];
	for (unsigned int t = 0; t < args.numthreads; t++)
		thr[t] = new thread(ReadWptThread, t, &highway_systems, &hs_it, &list_mtx, args.highwaydatapath+"/hwy_data",
				    &el, &all_wpt_files, &all_waypoints, &strtok_mtx, datacheckerrors);
	for (unsigned int t = 0; t < args.numthreads; t++)
		thr[t]->join();
	for (unsigned int t = 0; t < args.numthreads; t++)
		delete thr[t];
      #else
	for (HighwaySystem* h : highway_systems)
	{	std::cout << h->systemname << std::flush;
		for (std::list<Route>::iterator r = h->route_list.begin(); r != h->route_list.end(); r++)
		{	r->read_wpt(&all_waypoints, &el, args.highwaydatapath+"/hwy_data", &strtok_mtx, datacheckerrors, &all_wpt_files);
		}
		std::cout << "!" << std::endl;
	}
      #endif

	//#include "debug/qt_and_colocate_check.cpp"
	cout << et.et() << "Writing WaypointQuadtree.tmg." << endl;
	all_waypoints.write_qt_tmg(); //FIXME needs argument for file path
	cout << et.et() << "Sorting waypoints in Quadtree." << endl;
	all_waypoints.sort();
	cout << et.et() << "Sorting colocated point lists." << endl;
	for (Waypoint *w : all_waypoints.point_list())
		if (w->colocated && w == w->colocated->front()) w->colocated->sort(sort_root_at_label);
	//#include "debug/qt_and_colocate_check.cpp"

	cout << et.et() << "Finding unprocessed wpt files." << endl;
	if (all_wpt_files.size())
	{	ofstream unprocessedfile(args.logfilepath+"/unprocessedwpts.log");
		cout << all_wpt_files.size() << " .wpt files in " << args.highwaydatapath + "/hwy_data not processed, see unprocessedwpts.log." << endl;
		for (const string &f : all_wpt_files) unprocessedfile << strstr(f.data(), "hwy_data") << '\n';
		unprocessedfile.close();
		all_wpt_files.clear();
	}
	else	cout << "All .wpt files in " << args.highwaydatapath << "/hwy_data processed." << endl;

	// Near-miss point log
	cout << et.et() << "Near-miss point log and tm-master.nmp file." << endl;

	// read in fp file
	list<string> nmpfplist;
	file.open(args.highwaydatapath+"/nmpfps.log");
	while (getline(file, line))
	{	while (line.back() == 0x0D || line.back() == ' ') line.erase(line.end()-1);	// trim DOS newlines & whitespace
		if (line.size()) nmpfplist.push_back(line);
	}
	file.close();

	list<string> nmploglines;
	ofstream nmplog(args.logfilepath+"/nearmisspoints.log");
	ofstream nmpnmp(args.logfilepath+"/tm-master.nmp");
	for (Waypoint *w : all_waypoints.point_list()) w->nmplogs(nmpfplist, nmpnmp, nmploglines);
	nmpnmp.close();

	// sort and write actual lines to nearmisspoints.log
	nmploglines.sort();
	for (string &n : nmploglines) nmplog << n << '\n';
	nmploglines.clear();
	nmplog.close();

	// report any unmatched nmpfps.log entries
	ofstream nmpfpsunmatchedfile(args.logfilepath+"/nmpfpsunmatched.log");
	for (string &line : nmpfplist)
		nmpfpsunmatchedfile << line << '\n';
	nmpfpsunmatchedfile.close();
	nmpfplist.clear();

	// if requested, rewrite data with near-miss points merged in
	if (args.nmpmergepath != "" && !args.errorcheck)
	{	cout << et.et() << "Writing near-miss point merged wpt files." << endl; //FIXME output dots to indicate progress
	      #ifdef threading_enabled
		// set up for threaded nmp_merged file writes
		hs_it = highway_systems.begin();
		for (unsigned int t = 0; t < args.numthreads; t++)
			thr[t] = new thread(NmpMergedThread, t, &highway_systems, &hs_it, &list_mtx, &args.nmpmergepath);
		for (unsigned int t = 0; t < args.numthreads; t++)
			thr[t]->join();
		for (unsigned int t = 0; t < args.numthreads; t++)
			delete thr[t];//*/
	      #else
		for (HighwaySystem *h : highway_systems)
		  for (Route &r : h->route_list)
		    r.write_nmp_merged(args.nmpmergepath + "/" + r.region->code);
	      #endif
	}

	#include "functions/concurrency_detection.cpp"

	// Create hash table for faster lookup of routes by list file name
	cout << et.et() << "Creating route hash table for list processing:" << endl;
	unordered_map<string, Route*> route_hash;
	for (HighwaySystem *h : highway_systems)
	  for (list<Route>::iterator r = h->route_list.begin(); r != h->route_list.end(); r++) //FIXME use more compact syntax (&)
	  {	route_hash[lower(r->readable_name())] = &*r;
		for (string &a : r->alt_route_names) route_hash[lower(r->region->code + " " + a)] = &*r;
	  }

	// Create a list of TravelerList objects, one per person
	list<TravelerList*> traveler_lists;
	list<string>::iterator id_it = traveler_ids.begin();

	cout << et.et() << "Processing traveler list files:" << endl;
      #ifdef threading_enabled
	// set up for threaded .list file processing
	for (unsigned int t = 0; t < args.numthreads; t++)
		thr[t] = new thread(ReadListThread, t, &traveler_ids, &id_it, &traveler_lists, &list_mtx, &strtok_mtx, &args, &route_hash);
	for (unsigned int t = 0; t < args.numthreads; t++)
		thr[t]->join();
	for (unsigned int t = 0; t < args.numthreads; t++)
		delete thr[t];
      #else
	for (string &t : traveler_ids)
	{	cout << t << ' ' << std::flush;
		traveler_lists.push_back(new TravelerList(t, &route_hash, &args, &strtok_mtx));
	}
	traveler_ids.clear();
      #endif
	cout << endl << et.et() << "Processed " << traveler_lists.size() << " traveler list files." << endl;
	traveler_lists.sort(sort_travelers_by_name);
	// assign traveler numbers for master traveled graph
	unsigned int travnum = 0;
	for (TravelerList *t : traveler_lists)
	{	t->traveler_num[0] = travnum;
		travnum++;
	}

	//#include "debug/highway_segment_log.cpp"
	//#include "debug/pioneers.cpp"

	// Read updates.csv file, just keep in the fields list for now since we're
	// just going to drop this into the DB later anyway
	list<array<string, 5>> updates;
	cout << et.et() << "Reading updates file." << endl;
	file.open(args.highwaydatapath+"/updates.csv");
	getline(file, line); // ignore header line
	while (getline(file, line))
	{	if (line.back() == 0x0D) line.erase(line.end()-1);	// trim DOS newlines
		array<string, 5> fields;

		// date
		size_t left = line.find(';');
		if (left == std::string::npos)
		{	el.add_error("Could not parse updates.csv line: [" + line + "], expected 5 fields, found 1");
			continue;
		}
		fields[0] = line.substr(0,left);

		// region
		size_t right = line.find(';', left+1);
		if (right == std::string::npos)
		{	el.add_error("Could not parse updates.csv line: [" + line + "], expected 5 fields, found 2");
			continue;
		}
		fields[1] = line.substr(left+1, right-left-1);

		// route
		left = right;
		right = line.find(';', left+1);
		if (right == std::string::npos)
		{	el.add_error("Could not parse updates.csv line: [" + line + "], expected 5 fields, found 3");
			continue;
		}
		fields[2] = line.substr(left+1, right-left-1);

		// root
		left = right;
		right = line.find(';', left+1);
		if (right == std::string::npos)
		{	el.add_error("Could not parse updates.csv line: [" + line + "], expected 5 fields, found 4");
			continue;
		}
		fields[3] = line.substr(left+1, right-left-1);

		// description
		left = right;
		right = line.find(';', left+1);
		if (right != std::string::npos)
		{	el.add_error("Could not parse updates.csv line: [" + line + "], expected 5 fields, found more");
			continue;
		}
		fields[4] = line.substr(left+1);

		updates.push_back(fields);
	}
	file.close();

	// Same plan for systemupdates.csv file, again just keep in the fields
	// list for now since we're just going to drop this into the DB later
	// anyway
	list<array<string, 5>> systemupdates;
	cout << et.et() << "Reading systemupdates file." << endl;
	file.open(args.highwaydatapath+"/systemupdates.csv");
	getline(file, line);  // ignore header line
	while (getline(file, line))
	{	if (line.back() == 0x0D) line.erase(line.end()-1);	// trim DOS newlines
		array<string, 5> fields;

		// date
		size_t left = line.find(';');
		if (left == std::string::npos)
		{	el.add_error("Could not parse systemupdates.csv line: [" + line + "], expected 5 fields, found 1");
			continue;
		}
		fields[0] = line.substr(0,left);

		// region
		size_t right = line.find(';', left+1);
		if (right == std::string::npos)
		{	el.add_error("Could not parse systemupdates.csv line: [" + line + "], expected 5 fields, found 2");
			continue;
		}
		fields[1] = line.substr(left+1, right-left-1);

		// systemName
		left = right;
		right = line.find(';', left+1);
		if (right == std::string::npos)
		{	el.add_error("Could not parse systemupdates.csv line: [" + line + "], expected 5 fields, found 3");
			continue;
		}
		fields[2] = line.substr(left+1, right-left-1);

		// description
		left = right;
		right = line.find(';', left+1);
		if (right == std::string::npos)
		{	el.add_error("Could not parse systemupdates.csv line: [" + line + "], expected 5 fields, found 4");
			continue;
		}
		fields[3] = line.substr(left+1, right-left-1);

		// statusChange
		left = right;
		right = line.find(';', left+1);
		if (right != std::string::npos)
		{	el.add_error("Could not parse systemupdates.csv line: [" + line + "], expected 5 fields, found more");
			continue;
		}
		fields[4] = line.substr(left+1);

		systemupdates.push_back(fields);
	}
	file.close();

	// write log file for points in use -- might be more useful in the DB later,
	// or maybe in another format
	cout << et.et() << "Writing points in use log." << endl;
	ofstream inusefile(args.logfilepath+"/pointsinuse.log");
	timestamp = time(0);
	inusefile << "Log file created at: " << ctime(&timestamp);
	for (HighwaySystem *h : highway_systems)
	  for (Route &r : h->route_list)
	    if (r.labels_in_use.size())
	    {	inusefile << r.root << '(' << r.point_list.size() << "):";
		list<string> liu_list(r.labels_in_use.begin(), r.labels_in_use.end());
		liu_list.sort();
		for (string label : liu_list) inusefile << ' ' << label;
		inusefile << '\n';
		r.labels_in_use.clear();
	    }
	inusefile.close();

	// write log file for alt labels not in use
	cout << et.et() << "Writing unused alt labels log." << endl;
	ofstream unusedfile(args.logfilepath+"/unusedaltlabels.log");
	timestamp = time(0);
	unusedfile << "Log file created at: " << ctime(&timestamp);
	unsigned int total_unused_alt_labels = 0;
	for (HighwaySystem *h : highway_systems)
		for (Route &r : h->route_list)
			if (r.unused_alt_labels.size())
			{	total_unused_alt_labels += r.unused_alt_labels.size();
				unusedfile << r.root << '(' << r.unused_alt_labels.size() << "):";
				list<string> ual_list(r.unused_alt_labels.begin(), r.unused_alt_labels.end());
				ual_list.sort();
				for (string label : ual_list) unusedfile << ' ' << label;
				unusedfile << '\n';
				r.unused_alt_labels.clear();
			}
	unusedfile << "Total: " << total_unused_alt_labels << '\n';
	unusedfile.close();


	// now augment any traveler clinched segments for concurrencies
	cout << et.et() << "Augmenting travelers for detected concurrent segments." << flush;
        //#include "debug/concurrency_augments.cpp"
      #ifdef threading_enabled
	// set up for threaded concurrency augments
	list<TravelerList*>::iterator tl_it = traveler_lists.begin();

	for (unsigned int t = 0; t < args.numthreads; t++)
		thr[t] = new thread(ConcAugThread, t, &traveler_lists, &tl_it, &list_mtx, &log_mtx, &concurrencyfile);
	for (unsigned int t = 0; t < args.numthreads; t++)
		thr[t]->join();
	for (unsigned int t = 0; t < args.numthreads; t++)
		delete thr[t];//*/
      #else
	for (TravelerList *t : traveler_lists)
	{	cout << '.' << flush;
		for (HighwaySegment *s : t->clinched_segments)
		  if (s->concurrent)
		    for (HighwaySegment *hs : *(s->concurrent))
		      if (hs->route->system->active_or_preview() && hs->add_clinched_by(t))
			concurrencyfile << "Concurrency augment for traveler " << t->traveler_name << ": [" << hs->str() << "] based on [" << s->str() << "]\n";
	}
      #endif

	cout << "!\n";
	concurrencyfile.close();

	/*ofstream sanetravfile(args.logfilepath+"/concurrent_travelers_sanity_check.log");
	for (HighwaySystem *h : highway_systems)
	    for (Route &r : h->route_list)
		for (HighwaySegment *s : r.segment_list)
		    sanetravfile << s->concurrent_travelers_sanity_check();
	sanetravfile.close(); //*/

	// compute lots of stats, first total mileage by route, system, overall, where
	// system and overall are stored in unordered_maps by region
	cout << et.et() << "Computing stats." << flush;
	// now also keeping separate totals for active only, active+preview,
	// and all for overall (not needed for system, as a system falls into just
	// one of these categories)

      #ifdef threading_enabled
	// set up for threaded stats computation
	hs_it = highway_systems.begin();
	for (unsigned int t = 0; t < args.numthreads; t++)
		thr[t] = new thread(ComputeStatsThread, t, &highway_systems, &hs_it, &list_mtx);
	for (unsigned int t = 0; t < args.numthreads; t++)
		thr[t]->join();
	for (unsigned int t = 0; t < args.numthreads; t++)
		delete thr[t];//*/
      #else
	for (HighwaySystem *h : highway_systems)
	{	cout << "." << flush;
		for (Route &r : h->route_list)
		  for (HighwaySegment *s : r.segment_list)
		    s->compute_stats();
	}
      #endif
	cout << '!' << endl;

	cout << et.et() << "Writing highway data stats log file (highwaydatastats.log)." << endl;
	char fstr[112];
	ofstream hdstatsfile(args.logfilepath+"/highwaydatastats.log");
	timestamp = time(0);
	hdstatsfile << "Travel Mapping highway mileage as of " << ctime(&timestamp);

	double active_only_miles = 0;
	double active_preview_miles = 0;
	double overall_miles = 0;
	for (list<Region>::iterator r = all_regions.begin(); r != all_regions.end(); r++)  //FIXME brevity
	{	active_only_miles += r->active_only_mileage;
		active_preview_miles += r->active_preview_mileage;
		overall_miles += r->overall_mileage;
	}
	sprintf(fstr, "Active routes (active): %.2f mi\n", active_only_miles);
	hdstatsfile << fstr;
	sprintf(fstr, "Clinchable routes (active, preview): %.2f mi\n", active_preview_miles);
	hdstatsfile << fstr;
	sprintf(fstr, "All routes (active, preview, devel): %.2f mi\n", overall_miles);
	hdstatsfile << fstr;
	hdstatsfile << "Breakdown by region:\n";
	// let's sort alphabetically by region
	// a nice enhancement later here might break down by continent, then country,
	// then region
	list<string> region_entries;
	for (Region region : all_regions)
	  if (region.overall_mileage)
	  {	sprintf(fstr, ": %.2f (active), %.2f (active, preview) %.2f (active, preview, devel)\n",
			region.active_only_mileage, region.active_preview_mileage, region.overall_mileage);
		region_entries.push_back(region.code + fstr);
	  }
	region_entries.sort();
	for (string e : region_entries) hdstatsfile << e;

	for (HighwaySystem *h : highway_systems)
	{	sprintf(fstr, ") total: %.2f mi\n", h->total_mileage());
		hdstatsfile << "System " << h->systemname << " (" << h->level_name() << fstr;
		if (h->mileage_by_region.size() > 1)
		{	hdstatsfile << "System " << h->systemname << " by region:\n";
			list<Region*> regions_in_system;
			for (pair<Region*, double> rm : h->mileage_by_region)
				regions_in_system.push_back(rm.first);
			regions_in_system.sort(sort_regions_by_code);
			for (Region *r : regions_in_system)
			{	sprintf(fstr, ": %.2f mi\n", h->mileage_by_region.at(r));
				hdstatsfile << r->code << fstr;
			}
		}
		hdstatsfile << "System " << h->systemname << " by route:\n";
		for (list<ConnectedRoute>::iterator cr = h->con_route_list.begin(); cr != h->con_route_list.end(); cr++)
		{	double con_total_miles = 0;
			string to_write = "";
			for (Route *r : cr->roots)
			{	sprintf(fstr, ": %.2f mi\n", r->mileage);
				to_write += "  " + r->readable_name() + fstr;
				con_total_miles += r->mileage;
			}
			cr->mileage = con_total_miles; //FIXME?
			sprintf(fstr, ": %.2f mi", con_total_miles);
			hdstatsfile << cr->readable_name() << fstr;
			if (cr->roots.size() == 1)
				hdstatsfile << " (" << cr->roots[0]->readable_name() << " only)\n";
			else	hdstatsfile << '\n' << to_write;
		}
	}
	hdstatsfile.close();
	// this will be used to store DB entry lines as needed values are computed here,
	// to be added into the DB later in the program
	ClinchedDBValues clin_db_val;

	// now add user clinched stats to their log entries
	cout << et.et() << "Creating per-traveler stats logs and augmenting data structure." << flush;
      #ifdef threading_enabled
	// set up for threaded user logs
	tl_it = traveler_lists.begin();
	for (unsigned int t = 0; t < args.numthreads; t++)
		thr[t] = new thread
		(	UserLogThread, t, &traveler_lists, &tl_it, &list_mtx, &clin_db_val, active_only_miles,
			active_preview_miles, &highway_systems, args.logfilepath+"/users/"
		);
	for (unsigned int t = 0; t < args.numthreads; t++)
		thr[t]->join();
	for (unsigned int t = 0; t < args.numthreads; t++)
		delete thr[t];//*/
      #else
	for (TravelerList *t : traveler_lists) t->userlog(&clin_db_val, active_only_miles, active_preview_miles, &highway_systems, args.logfilepath+"/users/");
      #endif
	cout << "!" << endl;

	// write stats csv files
	cout << et.et() << "Writing stats csv files." << endl;
	double total_mi;

	// first, overall per traveler by region, both active only and active+preview
	ofstream allfile(args.csvstatfilepath + "/allbyregionactiveonly.csv");
	allfile << "Traveler,Total";
	std::list<Region*> regions;
	total_mi = 0;
	for (list<Region>::iterator r = all_regions.begin(); r != all_regions.end(); r++)
	  if (r->active_only_mileage)
	  {	regions.push_back(&*r);
		total_mi += r->active_only_mileage;
	  }
	regions.sort(sort_regions_by_code);
	for (Region *region : regions)
		allfile << ',' << region->code;
	allfile << '\n';
	for (TravelerList *t : traveler_lists)
	{	double t_total_mi = 0;
		for (std::pair<Region*, double> rm : t->active_only_mileage_by_region)
			t_total_mi += rm.second;
		sprintf(fstr, "%.2f", t_total_mi);
		allfile << t->traveler_name << ',' << fstr;
		for (Region *region : regions)
		  try {	sprintf(fstr, "%.2f", t->active_only_mileage_by_region.at(region));
			allfile << ',' << fstr;
		      }
		  catch (const std::out_of_range& oor)
		      {	allfile << ",0";
		      }
		allfile << '\n';
	}
	sprintf(fstr, "TOTAL,%.2f", total_mi);
	allfile << fstr;
	for (Region *region : regions)
	{	sprintf(fstr, ",%.2f", region->active_only_mileage);
		allfile << fstr;
	}
	allfile << '\n';
	allfile.close();

	// active+preview
	allfile.open(args.csvstatfilepath + "/allbyregionactivepreview.csv");
	allfile << "Traveler,Total";
	regions.clear();
	total_mi = 0;
	for (list<Region>::iterator r = all_regions.begin(); r != all_regions.end(); r++)
	  if (r->active_preview_mileage)
	  {	regions.push_back(&*r);
		total_mi += r->active_preview_mileage;
	  }
	regions.sort(sort_regions_by_code);
	for (Region *region : regions)
		allfile << ',' << region->code;
	allfile << '\n';
	for (TravelerList *t : traveler_lists)
	{	double t_total_mi = 0;
		for (std::pair<Region*, double> rm : t->active_preview_mileage_by_region)
			t_total_mi += rm.second;
		sprintf(fstr, "%.2f", t_total_mi);
		allfile << t->traveler_name << ',' << fstr;
		for (Region *region : regions)
		  try {	sprintf(fstr, "%.2f", t->active_preview_mileage_by_region.at(region));
			allfile << ',' << fstr;
		      }
		  catch (const std::out_of_range& oor)
		      {	allfile << ",0";
		      }
		allfile << '\n';
	}
	sprintf(fstr, "TOTAL,%.2f", total_mi);
	allfile << fstr;
	for (Region *region : regions)
	{	sprintf(fstr, ",%.2f", region->active_preview_mileage);
		allfile << fstr;
	}
	allfile << '\n';
	allfile.close();

	// now, a file for each system, again per traveler by region
	for (HighwaySystem *h : highway_systems)
	{	ofstream sysfile(args.csvstatfilepath + "/" + h->systemname + "-all.csv");
		sysfile << "Traveler,Total";
		regions.clear();
		total_mi = 0;
		for (std::pair<Region*, double> rm : h->mileage_by_region)
		{	regions.push_back(rm.first);
			total_mi += rm.second;		//TODO is this right?
		}
		regions.sort(sort_regions_by_code);
		for (Region *region : regions)
			sysfile << ',' << region->code;
		sysfile << '\n';
		for (TravelerList *t : traveler_lists)
		  // only include entries for travelers who have any mileage in system
		  if (t->system_region_mileages.find(h) != t->system_region_mileages.end())
		  {	sprintf(fstr, ",%.2f", t->system_region_miles(h));
			sysfile << t->traveler_name << fstr;
			for (Region *region : regions)
			  try {	sprintf(fstr, ",%.2f", t->system_region_mileages.at(h).at(region));
				sysfile << fstr;
			      }
			  catch (const std::out_of_range& oor)
			      {	sysfile << ",0";
			      }
			sysfile << '\n';
		  }
		sprintf(fstr, "TOTAL,%.2f", h->total_mileage());
		sysfile << fstr;
		for (Region *region : regions)
		{	sprintf(fstr, ",%.2f", h->mileage_by_region.at(region));
			sysfile << fstr;
		}
		sysfile << '\n';
		sysfile.close();
	}

	// read in the datacheck false positives list
	cout << et.et() << "Reading datacheckfps.csv." << endl;
	file.open(args.highwaydatapath+"/datacheckfps.csv");
	getline(file, line); // ignore header line
	list<array<string, 6>> datacheckfps; //FIXME try implementing as an unordered_multiset; see if speed increases
	unordered_set<string> datacheck_always_error
	({	"BAD_ANGLE", "DUPLICATE_LABEL", "HIDDEN_TERMINUS",
		"INVALID_FINAL_CHAR", "INVALID_FIRST_CHAR",
		"LABEL_INVALID_CHAR", "LABEL_PARENS", "LABEL_SLASHES",
		"LABEL_UNDERSCORES", "LONG_UNDERSCORE",
		"MALFORMED_LAT", "MALFORMED_LON", "MALFORMED_URL",
		"NONTERMINAL_UNDERSCORE"
	});
	while (getline(file, line))
	{	if (line.back() == 0x0D) line.erase(line.end()-1);	// trim DOS newlines
		array<string, 6> fields;

		// Root
		size_t left = line.find(';');
		if (left == std::string::npos)
		{	el.add_error("Could not parse datacheckfps.csv line: [" + line + "], expected 6 fields, found 1");
			continue;
		}
		fields[0] = line.substr(0,left);

		// Waypoint1
		size_t right = line.find(';', left+1);
		if (right == std::string::npos)
		{	el.add_error("Could not parse datacheckfps.csv line: [" + line + "], expected 6 fields, found 2");
			continue;
		}
		fields[1] = line.substr(left+1, right-left-1);

		// Waypoint2
		left = right;
		right = line.find(';', left+1);
		if (right == std::string::npos)
		{	el.add_error("Could not parse datacheckfps.csv line: [" + line + "], expected 6 fields, found 3");
			continue;
		}
		fields[2] = line.substr(left+1, right-left-1);

		// Waypoint3
		left = right;
		right = line.find(';', left+1);
		if (right == std::string::npos)
		{	el.add_error("Could not parse datacheckfps.csv line: [" + line + "], expected 6 fields, found 4");
			continue;
		}
		fields[3] = line.substr(left+1, right-left-1);

		// Error
		left = right;
		right = line.find(';', left+1);
		if (right == std::string::npos)
		{	el.add_error("Could not parse datacheckfps.csv line: [" + line + "], expected 6 fields, found 5");
			continue;
		}
		fields[4] = line.substr(left+1, right-left-1);

		// Info
		left = right;
		right = line.find(';', left+1);
		if (right != std::string::npos)
		{	el.add_error("Could not parse datacheckfps.csv line: [" + line + "], expected 6 fields, found more");
			continue;
		}
		fields[5] = line.substr(left+1);

		if (datacheck_always_error.find(fields[4]) != datacheck_always_error.end())
		{	cout << "datacheckfps.csv line not allowed (always error): " << line << endl;
			continue;
		}
		datacheckfps.push_back(fields);
	}
	file.close();

	// See if we have any errors that should be fatal to the site update process
	if (el.error_list.size())
	{	cout << "ABORTING due to " << el.error_list.size() << " errors:" << endl;
		for (unsigned int i = 0; i < el.error_list.size(); i++)
			cout << i+1 << ": " << el.error_list[i] << endl;
		return 0;
	}

      #ifdef threading_enabled
	std::cout << et.et() << "Start writing database file " << args.databasename << ".sql.\n" << std::flush;
	thread sqlthread;
	if   (!args.errorcheck)
	{	sqlthread=thread(sqlfile1, &et, &args, &all_regions, &continents, &countries, &highway_systems, &traveler_lists, &clin_db_val, &updates, &systemupdates);
	}
      #endif

	#include "functions/graph_generation.cpp"

	// now mark false positives
	datacheckerrors->entries.sort();
	cout << et.et() << "Marking datacheck false positives." << flush;
	ofstream fpfile(args.logfilepath+"/nearmatchfps.log");
	timestamp = time(0);
	fpfile << "Log file created at: " << ctime(&timestamp);
	unsigned int counter = 0;
	unsigned int fpcount = 0;
	for (list<DatacheckEntry>::iterator d = datacheckerrors->entries.begin(); d != datacheckerrors->entries.end(); d++)
	{	//cout << "Checking: " << d->str() << endl;
		counter++;
		if (counter % 1000 == 0) cout << '.' << flush;
		for (list<array<string, 6>>::iterator fp = datacheckfps.begin(); fp != datacheckfps.end(); fp++)
		  if (d->match_except_info(*fp))
		    if (d->info == (*fp)[5])
		    {	//cout << "Match!" << endl;
			d->fp = 1;
			fpcount++;
			datacheckfps.erase(fp);
			break;
		    }
		    else
		    {	fpfile << "FP_ENTRY: " << (*fp)[0] << ';' << (*fp)[1] << ';' << (*fp)[2] << ';' << (*fp)[3] << ';' << (*fp)[4] << ';' << (*fp)[5] << '\n';
			fpfile << "CHANGETO: " << (*fp)[0] << ';' << (*fp)[1] << ';' << (*fp)[2] << ';' << (*fp)[3] << ';' << (*fp)[4] << ';' << d->info << '\n';
		    }
	}
	fpfile.close();
	cout << '!' << endl;
	cout << et.et() << "Found " << datacheckerrors->entries.size() << " datacheck errors." << endl;
	cout << et.et() << "Matched " << fpcount << " FP entries." << endl;

	// write log of unmatched false positives from the datacheckfps.csv
	cout << et.et() << "Writing log of unmatched datacheck FP entries." << endl;
	fpfile.open(args.logfilepath+"/unmatchedfps.log");
	timestamp = time(0);
	fpfile << "Log file created at: " << ctime(&timestamp);
	if (datacheckfps.empty()) fpfile << "No unmatched FP entries.\n";
	else	for (array<string, 6> entry : datacheckfps)
			fpfile << entry[0] << ';' << entry[1] << ';' << entry[2] << ';' << entry[3] << ';' << entry[4] << ';' << entry[5] << '\n';
	fpfile.close();

	// datacheck.log file
	cout << et.et() << "Writing datacheck.log" << endl;
	ofstream logfile(args.logfilepath + "/datacheck.log");
	timestamp = time(0);
	logfile << "Log file created at: " << ctime(&timestamp);
	logfile << "Datacheck errors that have been flagged as false positives are not included.\n";
	logfile << "These entries should be in a format ready to paste into datacheckfps.csv.\n";
	logfile << "Root;Waypoint1;Waypoint2;Waypoint3;Error;Info\n";
	if (datacheckerrors->entries.empty()) logfile << "No datacheck errors found.\n";
	else	for (DatacheckEntry &d : datacheckerrors->entries)
		  if (!d.fp) logfile << d.str() << '\n';
	logfile.close();

	if   (args.errorcheck)
		cout << et.et() << "SKIPPING database file." << endl;
	else {
	      #ifdef threading_enabled
		sqlthread.join();
		std::cout << et.et() << "Resume writing database file " << args.databasename << ".sql.\n" << std::flush;
	      #else
		std::cout << et.et() << "Writing database file " << args.databasename << ".sql.\n" << std::flush;
		sqlfile1(&et, &args, &all_regions, &continents, &countries, &highway_systems, &traveler_lists, &clin_db_val, &updates, &systemupdates);
	      #endif
		sqlfile2(&et, &args, &graph_types, &graph_vector, datacheckerrors);
	     }

	// print some statistics
	cout << et.et() << "Processed " << highway_systems.size() << " highway systems." << endl;
	unsigned int routes = 0;
	unsigned int points = 0;
	unsigned int segments = 0;
	for (HighwaySystem *h : highway_systems)
	{	routes += h->route_list.size();
		for (Route &r : h->route_list)
		{	points += r.point_list.size();
			segments += r.segment_list.size();
		}
	}
	cout << "Processed " << routes << " routes with a total of " << points << " points and " << segments << " segments." << endl;
	if (points != all_waypoints.size())
	  cout << "MISMATCH: all_waypoints contains " << all_waypoints.size() << " waypoints!" << endl;
	cout << "WaypointQuadtree contains " << all_waypoints.total_nodes() << " total nodes." << endl;

	if (!args.errorcheck)
	{	// compute colocation of waypoints stats
		cout << et.et() << "Computing waypoint colocation stats, reporting all with 8 or more colocations:" << endl;
		unsigned int largest_colocate_count = all_waypoints.max_colocated();
		vector<unsigned int> colocate_counts(largest_colocate_count+1, 0);
		for (Waypoint *w : all_waypoints.point_list())
		{	unsigned int c = w->num_colocated();
			colocate_counts[c] += 1;
			if (c >= 8 && w == w->colocated->front())
			{	printf("(%.15g, %.15g) is occupied by %i waypoints: ['", w->lat, w->lng, c);
				list<Waypoint*>::iterator p = w->colocated->begin();
				cout << (*p)->route->root << ' ' << (*p)->label << '\'';
				for (p++; p != w->colocated->end(); p++)
					cout << ", '" << (*p)->route->root << ' ' << (*p)->label << '\'';
				cout << "]\n";//*/
			}
		}
		cout << "Waypoint colocation counts:" << endl;
		unsigned int unique_locations = 0;
		for (unsigned int c = 1; c <= largest_colocate_count; c++)
		{	unique_locations += colocate_counts[c]/c;
			printf("%6i are each occupied by %2i waypoints.\n", colocate_counts[c]/c, c);
		}
		cout << "Unique locations: " << unique_locations << endl;
	}

	/* EDB
	cout << endl;
	unsigned int a_count = 0;
	unsigned int p_count = 0;
	unsigned int d_count = 0;
	unsigned other_count = 0;
	unsigned int total_rtes = 0;
	for (HighwaySystem *h : highway_systems)
	  for (Route r : h->route_list)
	  {	total_rtes++;
		if (h->devel()) d_count++;
		else {	if (h->active()) a_count++;
			else if (h->preview()) p_count++;
			     else other_count++;
		     }
	  }
	cout << a_count+p_count << " clinchable routes:" << endl;
	cout << a_count << " in active systems, and" << endl;
	cout << p_count << " in preview systems." << endl;
	cout << d_count << " routes in devel systems." << endl;
	if (other_count) cout << other_count << " routes not designated active/preview/devel!" << endl;
	cout << total_rtes << " total routes." << endl;//*/

	if (args.errorcheck)
	    cout << "!!! DATA CHECK SUCCESSFUL !!!" << endl;

	timestamp = time(0);
	cout << "Finish: " << ctime(&timestamp);
	cout << "Total run time: " << et.et() << endl;

}
