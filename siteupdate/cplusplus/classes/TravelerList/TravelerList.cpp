#include "TravelerList.h"
#include "../Args/Args.h"
#include "../ConnectedRoute/ConnectedRoute.h"
#include "../DBFieldLength/DBFieldLength.h"
#include "../ErrorList/ErrorList.h"
#include "../HighwaySegment/HighwaySegment.h"
#include "../HighwaySystem/HighwaySystem.h"
#include "../Region/Region.h"
#include "../Route/Route.h"
#include "../Waypoint/Waypoint.h"
#include "../../functions/tmstring.h"
#include "../../templates/contains.cpp"
#include <cstring>

TravelerList::TravelerList(std::string& travname, ErrorList* el)
{	// initialize object variables
	traveler_num = new unsigned int[Args::numthreads];
		       // deleted by ~TravelerList
	traveler_num[0] = this - allusers.data; // init for master traveled graph
	traveler_name.assign(travname, 0, travname.size()-5); // strip ".list" from end of travname
	if (traveler_name.size() > DBFieldLength::traveler)
	  el->add_error("Traveler name " + traveler_name + " > " + std::to_string(DBFieldLength::traveler) + "bytes");

	// variables used in construction
	unsigned int list_entries = 0;
	std::ofstream splist;
	std::string update;
	if (Args::splitregionpath != "") splist.open(Args::splitregionpath+"/list_files/"+travname);

	// init user log
	std::ofstream log(Args::logfilepath+"/users/"+traveler_name+".log");
	time_t StartTime = time(0);
	log << "Log file created at: ";
	mtx.lock();
	log << ctime(&StartTime);
	mtx.unlock();
	// write last update date & time if known
	std::ifstream file(Args::userlistfilepath+"/../time_files/"+traveler_name+".time");
	if (file.is_open())
	{	getline(file, update);
		if (update.size())
		{	log << travname << " last updated: " << update << '\n';
			update.assign(update, 0, 10);
		}
		file.close();
	}

	// read .list file into memory
	  // we can't getline here because it only allows one delimiter, and we need two; '\r' and '\n'.
	  // at least one .list file contains newlines using only '\r' (0x0D):
	  // https://github.com/TravelMapping/UserData/blob/6309036c44102eb3325d49515b32c5eef3b3cb1e/list_files/whopperman.list
	file.open(Args::userlistfilepath+"/"+travname);
	if (!file.is_open())
	{	std::cout << "\nERROR: " << travname << " not found.";
		file_not_found = 1;
	}
	if (file_not_found)
		// We're going to abort, so no point in continuing to fully build out TravelerList objects.
		// Future constructors will proceed only this far, to get a complete list of invalid names.
		return;
	else	std::cout << traveler_name << ' ' << std::flush;
	file.seekg(0, std::ios::end);
	unsigned long listdatasize = file.tellg();
	file.seekg(0, std::ios::beg);
	char *listdata = new char[listdatasize+1];
			 // deleted after processing lines
	file.read(listdata, listdatasize);
	listdata[listdatasize] = 0; // add null terminator
	file.close();

	// get canonical newline for writing splitregion .list files
	std::string newline;
	unsigned long c = 0;
	while (listdata[c] != '\r' && listdata[c] != '\n' && c < listdatasize) c++;
	if (listdata[c] == '\r')
		if (listdata[c+1] == '\n')	newline = "\r\n";
		else				newline = "\r";
	else	if (listdata[c] == '\n')	newline = "\n";
	// Use CRLF as failsafe if .list file contains no newlines.
		else				newline = "\r\n";

	// separate listdata into series of lines & newlines
	std::vector<char*> lines;
	std::vector<std::string> endlines;
	size_t spn = 0;
	for (char *c = listdata; *c; c += spn)
	{	endlines.push_back("");
		for (spn = strcspn(c, "\n\r"); c[spn] == '\n' || c[spn] == '\r'; spn++)
		{	endlines.back().push_back(c[spn]);
			c[spn] = 0;
		}
		lines.push_back(c);
	}
	lines.push_back(listdata+listdatasize+1); // add a dummy "past-the-end" element to make lines[l+1]-2 work

	// strip UTF-8 byte order mark if present
	if (!strncmp(lines[0], "\xEF\xBB\xBF", 3))
	{	lines[0] += 3;
		splist << "\xEF\xBB\xBF";
	}

	// process lines
	for (unsigned int l = 0; l < lines.size()-1; l++)
	{	std::string orig_line(lines[l]);
		// strip whitespace
		while (lines[l][0] == ' ' || lines[l][0] == '\t') lines[l]++;
		char * endchar = lines[l+1]-2; // -2 skips over the 0 inserted while separating listdata into lines
		while (endchar > lines[l] && *endchar == 0) endchar--;  // skip back more for CRLF cases, and lines followed by blank lines
		if (endchar > lines[l])
		  while (*endchar == ' ' || *endchar == '\t')
		  {	*endchar = 0;
			endchar--;
		  }
		std::string trim_line(lines[l]);
		// ignore empty or "comment" lines
		if (lines[l][0] == 0 || lines[l][0] == '#')
		{	splist << orig_line << endlines[l];
			continue;
		}
		// process fields in line
		std::vector<char*> fields;
		size_t spn = 0;
		for (char* c = lines[l]; *c; c += spn)
		{	for (spn = strcspn(c, " \t"); c[spn] == ' ' || c[spn] == '\t'; spn++) c[spn] = 0;
			if (*c == '#') break;
			else fields.push_back(c);
		}
		#define UPDATE_NOTE(R) if (R->last_update) \
		{	updated_routes.insert(R); \
			log << "  Route updated " << R->last_update[0] << ": " << R->readable_name() << '\n'; \
		}
		if (fields.size() == 4)
		     {
			#include "mark_chopped_route_segments.cpp"
		     }
		else if (fields.size() == 6)
		     {
			#include "mark_connected_route_segments.cpp"
		     }
		else {	log << "Incorrect format line (4 or 6 fields expected, found "
			    << fields.size() << "): " << trim_line << '\n';
			splist << orig_line << endlines[l];
		     }
		#undef UPDATE_NOTE
	}
	delete[] listdata;
	log << "Processed " << list_entries << " good lines marking " << clinched_segments.size() << " segments traveled.\n";
	log.close();
	splist.close();
}

TravelerList::~TravelerList() {delete[] traveler_num;}

/* Return active mileage across all regions */
double TravelerList::active_only_miles()
{	double mi = 0;
	for (std::pair<Region* const, double>& rm : active_only_mileage_by_region) mi += rm.second;
	return mi;
}

/* Return active+preview mileage across all regions */
double TravelerList::active_preview_miles()
{	double mi = 0;
	for (std::pair<Region* const, double>& rm : active_preview_mileage_by_region) mi += rm.second;
	return mi;
}

/* Return mileage across all regions for a specified system */
double TravelerList::system_miles(HighwaySystem *h)
{	double mi = 0;
	for (std::pair<Region* const, double>& rm : system_region_mileages.at(h)) mi += rm.second;
	return mi;
}

std::mutex TravelerList::mtx;
std::list<std::string> TravelerList::ids;
std::list<std::string>::iterator TravelerList::id_it;
TMArray<TravelerList> TravelerList::allusers;
TravelerList* TravelerList::tl_it;
bool TravelerList::file_not_found = 0;
