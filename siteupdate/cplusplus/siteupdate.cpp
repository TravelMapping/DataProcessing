// Tab Width = 8

// Travel Mapping Project, Jim Teresco and Eric Bryant, 2015-2020
/* Code to read .csv and .wpt files and prepare for
adding to the Travel Mapping Project database.

(c) 2015-2020, Jim Teresco and Eric Bryant
Original Python version by Jim Teresco, with contributions from Eric Bryant and the TravelMapping team
C++ translation by Eric Bryant

This module defines classes to represent the contents of a
.csv file that lists the highways within a system, and a
.wpt file that lists the waypoints for a given highway.
*/

#include <cstring>
#include <dirent.h>
#include <thread>
#include "classes/Arguments/Arguments.h"
#include "classes/DBFieldLength/DBFieldLength.h"
#include "classes/ClinchedDBValues/ClinchedDBValues.h"
#include "classes/ConnectedRoute/ConnectedRoute.h"
#include "classes/DatacheckEntry/DatacheckEntry.h"
#include "classes/ElapsedTime/ElapsedTime.h"
#include "classes/ErrorList/ErrorList.h"
#include "classes/GraphGeneration/GraphListEntry.h"
#include "classes/GraphGeneration/HGEdge.h"
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
#include "functions/crawl_hwy_data.h"
#include "functions/split.h"
#include "functions/sql_file.h"
#include "functions/upper.h"
#include "templates/contains.cpp"
#ifdef threading_enabled
#include "threads/threads.h"
#endif
using namespace std;

int main(int argc, char *argv[])
{	ifstream file;
	string line;
	mutex list_mtx;
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
		else	el.add_error("Error opening user list file path \""+args.userlistfilepath+"\". (Not found?)");
	}
	else for (string &id : traveler_ids) id += ".list";

	// read region, country, continent descriptions
	cout << et.et() << "Reading region, country, and continent descriptions." << endl;

	// continents
	vector<pair<string, string>> continents;
	file.open(args.highwaydatapath+"/continents.csv");
	if (!file) el.add_error("Could not open "+args.highwaydatapath+"/continents.csv");
	else {	getline(file, line); // ignore header line
		while(getline(file, line))
		{	if (line.back() == 0x0D) line.erase(line.end()-1);	// trim DOS newlines
			if (line.empty()) continue;
			size_t delim = line.find(';');
			if (delim == string::npos)
			{	el.add_error("Could not parse continents.csv line: [" + line
					   + "], expected 2 fields, found 1");
				continue;
			}
			string code = line.substr(0,delim);
			string name = line.substr(delim+1);
			if (name.find(';') != string::npos)
			{	el.add_error("Could not parse continents.csv line: [" + line
					   + "], expected 2 fields, found 3");
				continue;
			}
			// verify field lengths
			if (code.size() > DBFieldLength::continentCode)
				el.add_error("Continent code > " + std::to_string(DBFieldLength::continentCode)
					   + " bytes in continents.csv line " + line);
			if (name.size() > DBFieldLength::continentName)
				el.add_error("Continent name > " + std::to_string(DBFieldLength::continentName)
					   + " bytes in continents.csv line " + line);
			continents.emplace_back(pair<string, string>(code, name));
		}
	     }
	file.close();
	// create a dummy continent to catch unrecognized continent codes in .csv files
	continents.emplace_back(pair<string, string>("error", "unrecognized continent code"));

	// countries
	vector<pair<string, string>> countries;
	file.open(args.highwaydatapath+"/countries.csv");
	if (!file) el.add_error("Could not open "+args.highwaydatapath+"/countries.csv");
	else {	getline(file, line); // ignore header line
		while(getline(file, line))
		{	if (line.back() == 0x0D) line.erase(line.end()-1);	// trim DOS newlines
			if (line.empty()) continue;
			size_t delim = line.find(';');
			if (delim == string::npos)
			{	el.add_error("Could not parse countries.csv line: [" + line
					   + "], expected 2 fields, found 1");
				continue;
			}
			string code = line.substr(0,delim);
			string name = line.substr(delim+1);
			if (name.find(';') != string::npos)
			{	el.add_error("Could not parse countries.csv line: [" + line
					   + "], expected 2 fields, found 3");
				continue;
			}
			// verify field lengths
			if (code.size() > DBFieldLength::countryCode)
				el.add_error("Country code > " + std::to_string(DBFieldLength::countryCode)
					   + " bytes in countries.csv line " + line);
			if (name.size() > DBFieldLength::countryName)
				el.add_error("Country name > " + std::to_string(DBFieldLength::countryName)
					   + " bytes in countries.csv line " + line);
			countries.emplace_back(pair<string, string>(code, name));
		}
	     }
	file.close();
	// create a dummy country to catch unrecognized country codes in .csv files
	countries.emplace_back(pair<string, string>("error", "unrecognized country code"));

	//regions
	vector<Region*> all_regions;
	unordered_map<string, Region*> region_hash;
	file.open(args.highwaydatapath+"/regions.csv");
	if (!file) el.add_error("Could not open "+args.highwaydatapath+"/regions.csv");
	else {	getline(file, line); // ignore header line
		while(getline(file, line))
		{	if (line.back() == 0x0D) line.erase(line.end()-1);	// trim DOS newlines
			if (line.empty()) continue;
			Region* r = new Region(line, countries, continents, el);
				    // deleted on termination of program
			if (r->is_valid)
			{	all_regions.push_back(r);
				region_hash[r->code] = r;
			} else	delete r;
		}
	     }
	file.close();
	// create a dummy region to catch unrecognized region codes in .csv files
	all_regions.push_back(new Region("error;unrecognized region code;error;error;unrecognized region code", countries, continents, el));
	region_hash[all_regions.back()->code] = all_regions.back();

	// Create a list of HighwaySystem objects, one per system in systems.csv file
	list<HighwaySystem*> highway_systems;
	cout << et.et() << "Reading systems list in " << args.highwaydatapath+"/"+args.systemsfile << "." << endl;
	file.open(args.highwaydatapath+"/"+args.systemsfile);
	if (!file) el.add_error("Could not open "+args.highwaydatapath+"/"+args.systemsfile);
	else {	getline(file, line); // ignore header line
		list<string> ignoring;
		while(getline(file, line))
		{	if (line.back() == 0x0D) line.erase(line.end()-1);	// trim DOS newlines
			if (line.empty()) continue;
			if (line[0] == '#')
			{	ignoring.push_back("Ignored comment in " + args.systemsfile + ": " + line);
				continue;
			}
			HighwaySystem *hs = new HighwaySystem(line, el, args.highwaydatapath+"/hwy_data/_systems", args.systemsfile, countries, region_hash);
					    // deleted on termination of program
			if (!hs->is_valid) delete hs;
			else {	highway_systems.push_back(hs);
			     }
		}
		cout << endl;
		// at the end, print the lines ignored
		for (string& l : ignoring) cout << l << endl;
		ignoring.clear();
	     }
	file.close();

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
				    &el, &all_wpt_files, &all_waypoints);
	for (unsigned int t = 0; t < args.numthreads; t++)
		thr[t]->join();
	for (unsigned int t = 0; t < args.numthreads; t++)
		delete thr[t];
      #else
	for (HighwaySystem* h : highway_systems)
	{	std::cout << h->systemname << std::flush;
		bool usa_flag = h->country->first == "USA";
		for (Route* r : h->route_list)
			r->read_wpt(&all_waypoints, &el, args.highwaydatapath+"/hwy_data", usa_flag, &all_wpt_files);
		std::cout << "!" << std::endl;
	}
      #endif

	//#include "debug/qt_and_colocate_check.cpp"
	cout << et.et() << "Writing WaypointQuadtree.tmg." << endl;
	all_waypoints.write_qt_tmg(args.logfilepath+"/WaypointQuadtree.tmg");
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
		list<string> all_wpts_list(all_wpt_files.begin(), all_wpt_files.end());
		all_wpts_list.sort();
		for (const string &f : all_wpts_list) unprocessedfile << strstr(f.data(), "hwy_data") << '\n';
		unprocessedfile.close();
		all_wpt_files.clear();
	}
	else	cout << "All .wpt files in " << args.highwaydatapath << "/hwy_data processed." << endl;

	// create NMP lists
	cout << et.et() << "Searching for near-miss points." << endl;
      #ifdef threading_enabled
	// set up for threaded NMP search
	hs_it = highway_systems.begin();

	for (unsigned int t = 0; t < args.numthreads; t++)
		thr[t] = new thread(NmpSearchThread, t, &highway_systems, &hs_it, &list_mtx, &all_waypoints);
	for (unsigned int t = 0; t < args.numthreads; t++)
		thr[t]->join();
	for (unsigned int t = 0; t < args.numthreads; t++)
		delete thr[t];
      #else
	for (Waypoint *w : all_waypoints.point_list()) w->near_miss_points = all_waypoints.near_miss_waypoints(w, 0.0005);
      #endif

	// Near-miss point log
	cout << et.et() << "Near-miss point log and tm-master.nmp file." << endl;

	// read in fp file
	unordered_set<string> nmpfps;
	file.open(args.highwaydatapath+"/nmpfps.log");
	while (getline(file, line))
	{	while (line.back() == 0x0D || line.back() == ' ') line.erase(line.end()-1);	// trim DOS newlines & whitespace
		if (line.size()) nmpfps.insert(line);
	}
	file.close();

	list<string> nmploglines;
	ofstream nmplog(args.logfilepath+"/nearmisspoints.log");
	ofstream nmpnmp(args.logfilepath+"/tm-master.nmp");
	for (Waypoint *w : all_waypoints.point_list()) w->nmplogs(nmpfps, nmpnmp, nmploglines);
	nmpnmp.close();

	// sort and write actual lines to nearmisspoints.log
	nmploglines.sort();
	for (string &n : nmploglines) nmplog << n << '\n';
	nmploglines.clear();
	nmplog.close();

	// report any unmatched nmpfps.log entries
	ofstream nmpfpsunmatchedfile(args.logfilepath+"/nmpfpsunmatched.log");
	list<string> nmpfplist(nmpfps.begin(), nmpfps.end());
	nmpfplist.sort();
	for (string &line : nmpfplist)
		nmpfpsunmatchedfile << line << '\n';
	nmpfpsunmatchedfile.close();
	nmpfplist.clear();
	nmpfps.clear();

	// if requested, rewrite data with near-miss points merged in
	if (args.nmpmergepath != "" && !args.errorcheck)
	{	cout << et.et() << "Writing near-miss point merged wpt files." << endl;
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
		{	std::cout << h->systemname << std::flush;
			for (Route *r : h->route_list)
			  r->write_nmp_merged(args.nmpmergepath + "/" + r->rg_str);
			std::cout << '.' << std::flush;
		}
	      #endif
		cout << endl;
	}

	#include "tasks/concurrency_detection.cpp"

	cout << et.et() << "Processing waypoint labels and checking for unconnected chopped routes." << endl;
	for (HighwaySystem* h : highway_systems)
	  for (Route* r : h->route_list)
	  {	// check for unconnected chopped routes
		if (!r->con_route)
		{	el.add_error(r->system->systemname + ".csv: root " + r->root + " not matched by any connected route root.");
			continue;
		}

		// check for mismatched route endpoints within connected routes
		#define q r->con_route->roots[r->rootOrder-1]
		if ( r->rootOrder > 0 && q->point_list.size() > 1 && r->point_list.size() > 1 && !r->con_beg()->same_coords(q->con_end()) )
		{	if ( q->con_beg()->same_coords(r->con_beg()) )
			{	//std::cout << "DEBUG: marking only " << q->str() << " reversed" << std::endl;
				//if (q->is_reversed) std::cout << "DEBUG: " << q->str() << " already reversed!" << std::endl;
				q->is_reversed = 1;
			}
			else if ( q->con_end()->same_coords(r->con_end()) )
			{	//std::cout << "DEBUG: marking only " << r->str() << " reversed" << std::endl;
				//if (r->is_reversed) std::cout << "DEBUG: " << r->str() << " already reversed!" << std::endl;
				r->is_reversed = 1;
			}
			else if ( q->con_beg()->same_coords(r->con_end()) )
			{	//std::cout << "DEBUG: marking both " << q->str() << " and " << r->str() << " reversed" << std::endl;
				//if (q->is_reversed) std::cout << "DEBUG: " << q->str() << " already reversed!" << std::endl;
				//if (r->is_reversed) std::cout << "DEBUG: " << r->str() << " already reversed!" << std::endl;
				q->is_reversed = 1;
				r->is_reversed = 1;
			}
			else
			{	DatacheckEntry::add(r, r->con_beg()->label, "", "",
						     "DISCONNECTED_ROUTE", q->con_end()->root_at_label());
				DatacheckEntry::add(q, q->con_end()->label, "", "",
						     "DISCONNECTED_ROUTE", r->con_beg()->root_at_label());
			}
		}
		#undef q

		// create label hashes and check for duplicates
		#define w r->point_list[index]
		for (unsigned int index = 0; index < r->point_list.size(); index++)
		{	// ignore case and leading '+' or '*'
			std::string upper_label = upper(w->label);
			while (upper_label[0] == '+' || upper_label[0] == '*')
				upper_label = upper_label.substr(1);
			// if primary label not duplicated, add to r->pri_label_hash
			if (r->alt_label_hash.find(upper_label) != r->alt_label_hash.end())
			{	DatacheckEntry::add(r, upper_label, "", "", "DUPLICATE_LABEL", "");
				r->duplicate_labels.insert(upper_label);
			}
			else if (!r->pri_label_hash.insert(std::pair<std::string, unsigned int>(upper_label, index)).second)
			{	DatacheckEntry::add(r, upper_label, "", "", "DUPLICATE_LABEL", "");
				r->duplicate_labels.insert(upper_label);
			}
			for (std::string& a : w->alt_labels)
			{	// create canonical AltLabels
				while (a[0] == '+' || a[0] == '*') a = a.substr(1);
				upper(a.data());
				// populate unused set
				r->unused_alt_labels.insert(a);
				// create label->index hashes and check if AltLabels duplicated
				if (r->pri_label_hash.find(a) != r->pri_label_hash.end())
				{	DatacheckEntry::add(r, a, "", "", "DUPLICATE_LABEL", "");
					r->duplicate_labels.insert(a);
				}
				else if (!r->alt_label_hash.insert(std::pair<std::string, unsigned int>(a, index)).second)
				{	DatacheckEntry::add(r, a, "", "", "DUPLICATE_LABEL", "");
					r->duplicate_labels.insert(a);
				}
			}
		}
		#undef w
	  }

	#include "tasks/read_updates.cpp"

	// Read most recent update dates/times for .list files
	// one line for each traveler, containing 4 space-separated fields:
	// 0: username with .list extension
	// 1-3: date, time, and time zone as written by "git log -n 1 --pretty=%ci"
	unordered_map<string, string**> listupdates;
	file.open("listupdates.txt");
	if (file.is_open()) cout << et.et() << "Reading .list updates file." << endl;
	while(getline(file, line))
	{	size_t NumFields = 4;
		string** fields = new string*[4];
		fields[0] = new string;	// deleted upon construction of unordered_map element
		fields[1] = new string;	// stays in TravelerList object
		fields[2] = new string;	// deleted once written to user log
		fields[3] = new string;	// deleted once written to user log
		split(line, fields, NumFields, ' ');
		if (NumFields != 4)
		{	cout << "WARNING: Could not parse listupdates.txt line: [" << line
			     << "], expected 4 fields, found " << std::to_string(NumFields) << endl;
			delete fields[0];
			delete fields[1];
			delete fields[2];
			delete fields[3];
			delete[] fields;
			continue;
		}
		listupdates[*fields[0]] = fields;
		delete fields[0];
	}
	file.close();

	// Create a list of TravelerList objects, one per person
	list<TravelerList*> traveler_lists;
	list<string>::iterator id_it = traveler_ids.begin();

	cout << et.et() << "Processing traveler list files:" << endl;
      #ifdef threading_enabled
	// set up for threaded .list file processing
	for (unsigned int t = 0; t < args.numthreads; t++)
		thr[t] = new thread(ReadListThread, t, &traveler_ids, &listupdates, &id_it, &traveler_lists, &list_mtx, &args, &el);
	for (unsigned int t = 0; t < args.numthreads; t++)
		thr[t]->join();
	for (unsigned int t = 0; t < args.numthreads; t++)
		delete thr[t];
      #else
	for (string &t : traveler_ids)
	{	cout << t << ' ' << std::flush;
		std::string** update;
		try {	update = listupdates.at(t);
		    }
		catch (const std::out_of_range& oor)
		    {	update = 0;
		    }
		traveler_lists.push_back(new TravelerList(t, update, &el, &args));
	}
      #endif
	traveler_ids.clear();
	listupdates.clear();
	cout << endl << et.et() << "Processed " << traveler_lists.size() << " traveler list files." << endl;
	traveler_lists.sort(sort_travelers_by_name);
	// assign traveler numbers for master traveled graph
	unsigned int travnum = 0;
	for (TravelerList *t : traveler_lists)
	{	t->traveler_num[0] = travnum;
		travnum++;
	}

	cout << et.et() << "Clearing route & label hash tables." << endl;
	Route::root_hash.clear();
	Route::pri_list_hash.clear();
	Route::alt_list_hash.clear();
	for (HighwaySystem* h : highway_systems)
	  for (Route* r : h->route_list)
	  {	r->pri_label_hash.clear();
		r->alt_label_hash.clear();
		r->duplicate_labels.clear();
	  }

	cout << et.et() << "Writing pointsinuse.log, unusedaltlabels.log, listnamesinuse.log and unusedaltroutenames.log" << endl;
	unsigned int total_unused_alt_labels = 0;
	unsigned int total_unusedaltroutenames = 0;
	list<string> unused_alt_labels;
	ofstream piufile(args.logfilepath+"/pointsinuse.log");
	ofstream lniufile(args.logfilepath+"/listnamesinuse.log");
	ofstream uarnfile(args.logfilepath+"/unusedaltroutenames.log");
	timestamp = time(0);
	piufile << "Log file created at: " << ctime(&timestamp);
	lniufile << "Log file created at: " << ctime(&timestamp);
	uarnfile << "Log file created at: " << ctime(&timestamp);
	for (HighwaySystem *h : highway_systems)
	{	for (Route *r : h->route_list)
		{	// labelsinuse.log line
			if (r->labels_in_use.size())
			{	piufile << r->root << '(' << r->point_list.size() << "):";
				list<string> liu_list(r->labels_in_use.begin(), r->labels_in_use.end());
				liu_list.sort();
				for (string& label : liu_list) piufile << ' ' << label;
				piufile << '\n';
				r->labels_in_use.clear();
			}
			// unusedaltlabels.log lines, to be sorted by root later
			if (r->unused_alt_labels.size())
			{	total_unused_alt_labels += r->unused_alt_labels.size();
				string ual_entry = r->root + '(' + to_string(r->unused_alt_labels.size()) + "):";
				list<string> ual_list(r->unused_alt_labels.begin(), r->unused_alt_labels.end());
				ual_list.sort();
				for (string& label : ual_list) ual_entry += ' ' + label;
				unused_alt_labels.push_back(ual_entry);
				r->unused_alt_labels.clear();
			}
		}
		// listnamesinuse.log line
		if (h->listnamesinuse.size())
		{	lniufile << h->systemname << '(' << h->route_list.size() << "):";
			list<string> lniu_list(h->listnamesinuse.begin(), h->listnamesinuse.end());
			lniu_list.sort();
			for (string& list_name : lniu_list) lniufile << " \"" << list_name << '"';
			lniufile << '\n';
			h->listnamesinuse.clear();
		}
		// unusedaltroutenames.log line
		if (h->unusedaltroutenames.size())
		{	total_unusedaltroutenames += h->unusedaltroutenames.size();
			uarnfile << h->systemname << '(' << h->unusedaltroutenames.size() << "):";
			list<string> uarn_list(h->unusedaltroutenames.begin(), h->unusedaltroutenames.end());
			uarn_list.sort();
			for (string& list_name : uarn_list) uarnfile << " \"" << list_name << '"';
			uarnfile << '\n';
			h->unusedaltroutenames.clear();
		}
	}
	piufile.close();
	lniufile.close();
	uarnfile << "Total: " << total_unusedaltroutenames << '\n';
	uarnfile.close();
	// sort lines and write unusedaltlabels.log
	unused_alt_labels.sort();
	ofstream ualfile(args.logfilepath+"/unusedaltlabels.log");
	timestamp = time(0);
	ualfile << "Log file created at: " << ctime(&timestamp);
	for (string &ual_entry : unused_alt_labels) ualfile << ual_entry << '\n';
	unused_alt_labels.clear();
	ualfile << "Total: " << total_unused_alt_labels << '\n';
	ualfile.close();


	// now augment any traveler clinched segments for concurrencies
	cout << et.et() << "Augmenting travelers for detected concurrent segments." << flush;
        //#include "debug/concurrency_augments.cpp"
      #ifdef threading_enabled
	// set up for threaded concurrency augments
	list<string>* augment_lists = new list<string>[args.numthreads];
	list<TravelerList*>::iterator tl_it = traveler_lists.begin();

	for (unsigned int t = 0; t < args.numthreads; t++)
		thr[t] = new thread(ConcAugThread, t, &traveler_lists, &tl_it, &list_mtx, augment_lists+t);
	for (unsigned int t = 0; t < args.numthreads; t++)
		thr[t]->join();
	cout << "!\n" << et.et() << "Writing to concurrencies.log." << endl;
	for (unsigned int t = 0; t < args.numthreads; t++)
	{	for (std::string& entry : augment_lists[t]) concurrencyfile << entry << '\n';
		delete thr[t];//*/
	}
	delete[] augment_lists;
      #else
	for (TravelerList *t : traveler_lists)
	{	cout << '.' << flush;
		for (HighwaySegment *s : t->clinched_segments)
		  if (s->concurrent)
		    for (HighwaySegment *hs : *(s->concurrent))
		      if (hs != s && hs->route->system->active_or_preview() && hs->add_clinched_by(t))
		       	concurrencyfile << "Concurrency augment for traveler " << t->traveler_name << ": [" << hs->str() << "] based on [" << s->str() << "]\n";
	}
	cout << '!' << endl;
      #endif
	concurrencyfile.close();

	/*ofstream sanetravfile(args.logfilepath+"/concurrent_travelers_sanity_check.log");
	for (HighwaySystem *h : highway_systems)
	    for (Route *r : h->route_list)
		for (HighwaySegment *s : r->segment_list)
		    sanetravfile << s->concurrent_travelers_sanity_check();
	sanetravfile.close(); //*/

	// compute lots of regional stats:
	// overall, active+preview, active only,
	// and per-system which falls into just one of these categories
      #ifdef threading_enabled
	// set up for threaded stats computation
	cout << et.et() << "Computing stats per route." << flush;
	hs_it = highway_systems.begin();
	for (unsigned int t = 0; t < args.numthreads; t++)
		thr[t] = new thread(CompStatsRThread, t, &highway_systems, &hs_it, &list_mtx);
	for (unsigned int t = 0; t < args.numthreads; t++)
		thr[t]->join();
	for (unsigned int t = 0; t < args.numthreads; t++)
		delete thr[t];//*/
	cout << '!' << endl;
	cout << et.et() << "Computing stats per traveler." << flush;
	tl_it = traveler_lists.begin();
	for (unsigned int t = 0; t < args.numthreads; t++)
		thr[t] = new thread(CompStatsTThread, t, &traveler_lists, &tl_it, &list_mtx);
	for (unsigned int t = 0; t < args.numthreads; t++)
		thr[t]->join();
	for (unsigned int t = 0; t < args.numthreads; t++)
		delete thr[t];
	cout << '!' << endl;
      #else
	cout << et.et() << "Computing stats." << flush;
	for (HighwaySystem *h : highway_systems)
	{	cout << "." << flush;
		for (Route *r : h->route_list)
		{ 	r->compute_stats_r();
		  	for (HighwaySegment *s : r->segment_list)
			  for (TravelerList *t : s->clinched_by)
			    s->compute_stats_t(t);
		}
	}
	cout << '!' << endl;
      #endif

	cout << et.et() << "Writing highway data stats log file (highwaydatastats.log)." << endl;
	char fstr[112];
	ofstream hdstatsfile(args.logfilepath+"/highwaydatastats.log");
	timestamp = time(0);
	hdstatsfile << "Travel Mapping highway mileage as of " << ctime(&timestamp);

	double active_only_miles = 0;
	double active_preview_miles = 0;
	double overall_miles = 0;
	for (Region* r : all_regions)
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
	for (Region* region : all_regions)
	  if (region->overall_mileage)
	  {	sprintf(fstr, ": %.2f (active), %.2f (active, preview) %.2f (active, preview, devel)\n",
			region->active_only_mileage, region->active_preview_mileage, region->overall_mileage);
		region_entries.push_back(region->code + fstr);
	  }
	region_entries.sort();
	for (string& e : region_entries) hdstatsfile << e;

	for (HighwaySystem *h : highway_systems)
	{	sprintf(fstr, ") total: %.2f mi\n", h->total_mileage());
		hdstatsfile << "System " << h->systemname << " (" << h->level_name() << fstr;
		if (h->mileage_by_region.size() > 1)
		{	hdstatsfile << "System " << h->systemname << " by region:\n";
			list<Region*> regions_in_system;
			for (pair<Region* const, double>& rm : h->mileage_by_region)
				regions_in_system.push_back(rm.first);
			regions_in_system.sort(sort_regions_by_code);
			for (Region *r : regions_in_system)
			{	sprintf(fstr, ": %.2f mi\n", h->mileage_by_region.at(r));
				hdstatsfile << r->code << fstr;
			}
		}
		hdstatsfile << "System " << h->systemname << " by route:\n";
		for (ConnectedRoute* cr : h->con_route_list)
		{	string to_write = "";
			for (Route *r : cr->roots)
			{	sprintf(fstr, ": %.2f mi\n", r->mileage);
				to_write += "  " + r->readable_name() + fstr;
				cr->mileage += r->mileage;
			}
			sprintf(fstr, ": %.2f mi", cr->mileage);
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
	for (Region* r : all_regions)
	  if (r->active_only_mileage)
	  {	regions.push_back(r);
		total_mi += r->active_only_mileage;
	  }
	regions.sort(sort_regions_by_code);
	for (Region *region : regions)
		allfile << ',' << region->code;
	allfile << '\n';
	for (TravelerList *t : traveler_lists)
	{	double t_total_mi = 0;
		for (std::pair<Region* const, double>& rm : t->active_only_mileage_by_region)
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
	for (Region* r : all_regions)
	  if (r->active_preview_mileage)
	  {	regions.push_back(r);
		total_mi += r->active_preview_mileage;
	  }
	regions.sort(sort_regions_by_code);
	for (Region *region : regions)
		allfile << ',' << region->code;
	allfile << '\n';
	for (TravelerList *t : traveler_lists)
	{	double t_total_mi = 0;
		for (std::pair<Region* const, double>& rm : t->active_preview_mileage_by_region)
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
		for (std::pair<Region* const, double>& rm : h->mileage_by_region)
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
	list<array<string, 6>> datacheckfps;
	unordered_set<string> datacheck_always_error
	({	"BAD_ANGLE", "DISCONNECTED_ROUTE", "DUPLICATE_LABEL",
		"HIDDEN_TERMINUS", "INTERSTATE_NO_HYPHEN",
		"INVALID_FINAL_CHAR", "INVALID_FIRST_CHAR",
		"LABEL_INVALID_CHAR", "LABEL_PARENS", "LABEL_SLASHES",
		"LABEL_TOO_LONG", "LABEL_UNDERSCORES", "LONG_UNDERSCORE",
		"MALFORMED_LAT", "MALFORMED_LON", "MALFORMED_URL",
		"NONTERMINAL_UNDERSCORE", "US_LETTER"
	});
	while (getline(file, line))
	{	// trim DOS newlines & trailing whitespace
		while (line.back() == 0x0D || line.back() == ' ' || line.back() == '\t')
			line.pop_back();
		// trim leading whitespace
		while (line[0] == ' ' || line[0] == '\t')
			line = line.substr(1);
		if (line.empty()) continue;
		// parse system updates.csv line
		size_t NumFields = 6;
		array<string, 6> fields;
		string* ptr_array[6] = {&fields[0], &fields[1], &fields[2], &fields[3], &fields[4], &fields[5]};
		split(line, ptr_array, NumFields, ';');
		if (NumFields != 6)
		{	el.add_error("Could not parse datacheckfps.csv line: [" + line
				   + "], expected 6 fields, found " + std::to_string(NumFields));
			continue;
		}
		if (datacheck_always_error.find(fields[4]) != datacheck_always_error.end())
			cout << "datacheckfps.csv line not allowed (always error): " << line << endl;
		else	datacheckfps.push_back(fields);
	}
	file.close();

      #ifdef threading_enabled
	thread sqlthread;
	if   (!args.errorcheck)
	{	std::cout << et.et() << "Start writing database file " << args.databasename << ".sql.\n" << std::flush;
		sqlthread=thread(sqlfile1, &et, &args, &all_regions, &continents, &countries, &highway_systems, &traveler_lists, &clin_db_val, &updates, &systemupdates);
	}
      #endif

	#include "tasks/graph_generation.cpp"

	// now mark false positives
	DatacheckEntry::errors.sort();
	cout << et.et() << "Marking datacheck false positives." << flush;
	ofstream fpfile(args.logfilepath+"/nearmatchfps.log");
	timestamp = time(0);
	fpfile << "Log file created at: " << ctime(&timestamp);
	unsigned int counter = 0;
	unsigned int fpcount = 0;
	for (DatacheckEntry& d : DatacheckEntry::errors)
	{	//cout << "Checking: " << d->str() << endl;
		counter++;
		if (counter % 1000 == 0) cout << '.' << flush;
		for (list<array<string, 6>>::iterator fp = datacheckfps.begin(); fp != datacheckfps.end(); fp++)
		  if (d.match_except_info(*fp))
		    if (d.info == (*fp)[5])
		    {	//cout << "Match!" << endl;
			d.fp = 1;
			fpcount++;
			datacheckfps.erase(fp);
			break;
		    }
		    else
		    {	fpfile << "FP_ENTRY: " << (*fp)[0] << ';' << (*fp)[1] << ';' << (*fp)[2] << ';' << (*fp)[3] << ';' << (*fp)[4] << ';' << (*fp)[5] << '\n';
			fpfile << "CHANGETO: " << (*fp)[0] << ';' << (*fp)[1] << ';' << (*fp)[2] << ';' << (*fp)[3] << ';' << (*fp)[4] << ';' << d.info << '\n';
		    }
	}
	fpfile.close();
	cout << '!' << endl;
	cout << et.et() << "Found " << DatacheckEntry::errors.size() << " datacheck errors and matched " << fpcount << " FP entries." << endl;

	// write log of unmatched false positives from the datacheckfps.csv
	cout << et.et() << "Writing log of unmatched datacheck FP entries." << endl;
	fpfile.open(args.logfilepath+"/unmatchedfps.log");
	timestamp = time(0);
	fpfile << "Log file created at: " << ctime(&timestamp);
	if (datacheckfps.empty()) fpfile << "No unmatched FP entries.\n";
	else	for (array<string, 6>& entry : datacheckfps)
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
	if (DatacheckEntry::errors.empty()) logfile << "No datacheck errors found.\n";
	else	for (DatacheckEntry &d : DatacheckEntry::errors)
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
		sqlfile2(&et, &args, &graph_types, &graph_vector);
	     }

	// See if we have any errors that should be fatal to the site update process
	if (el.error_list.size())
	{	cout << et.et() << "ABORTING due to " << el.error_list.size() << " errors:" << endl;
		for (unsigned int i = 0; i < el.error_list.size(); i++)
			cout << i+1 << ": " << el.error_list[i] << endl;
		return 1;
	}

	// print some statistics
	cout << et.et() << "Processed " << highway_systems.size() << " highway systems." << endl;
	unsigned int routes = 0;
	unsigned int points = 0;
	unsigned int segments = 0;
	for (HighwaySystem *h : highway_systems)
	{	routes += h->route_list.size();
		for (Route *r : h->route_list)
		{	points += r->point_list.size();
			segments += r->segment_list.size();
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
	  for (Route* r : h->route_list)
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
