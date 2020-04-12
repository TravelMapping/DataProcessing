class TravelerList
{   /* This class encapsulates the contents of one .list file
    that represents the travels of one individual user.

    A list file consists of lines of 4 values:
    region route_name start_waypoint end_waypoint

    which indicates that the user has traveled the highway names
    route_name in the given region between the waypoints named
    start_waypoint end_waypoint
    */
	public:
	std::mutex ap_mi_mtx, ao_mi_mtx, sr_mi_mtx;
	std::unordered_set<HighwaySegment*> clinched_segments;
	std::string traveler_name;
	std::unordered_map<Region*, double> active_preview_mileage_by_region;				// total mileage per region, active+preview only
	std::unordered_map<Region*, double> active_only_mileage_by_region;				// total mileage per region, active only
	std::unordered_map<HighwaySystem*, std::unordered_map<Region*, double>> system_region_mileages;	// mileage per region per system
	std::unordered_map<HighwaySystem*, std::unordered_map<ConnectedRoute*, double>> con_routes_traveled; // mileage per ConRte per system
														// TODO is this necessary?
														// ConRtes by definition exist in one system only
	std::unordered_map<Route*, double> routes_traveled;						// mileage per traveled route
	std::unordered_map<HighwaySystem*, unsigned int> con_routes_clinched;				// clinch count per system
	//std::unordered_map<HighwaySystem*, unsigned int> routes_clinched;				// commented out in original siteupdate.py
	unsigned int *traveler_num;
	unsigned int active_systems_traveled;
	unsigned int active_systems_clinched;
	unsigned int preview_systems_traveled;
	unsigned int preview_systems_clinched;
	static std::mutex alltrav_mtx;	// for locking the traveler_lists list when reading .lists from disk

	TravelerList(std::string travname, std::unordered_map<std::string, Route*> *route_hash, ErrorList *el, Arguments *args, std::mutex *strtok_mtx)
	{	active_systems_traveled = 0;
		active_systems_clinched = 0;
		preview_systems_traveled = 0;
		preview_systems_clinched = 0;
		unsigned int list_entries = 0;
		traveler_num = new unsigned int[args->numthreads];
			       // deleted on termination of program
		traveler_name = travname.substr(0, travname.size()-5); // strip ".list" from end of travname
		if (traveler_name.size() > DBFieldLength::traveler)
		  el->add_error("Traveler name " + traveler_name + " > " + std::to_string(DBFieldLength::traveler) + "bytes");
		std::ofstream log(args->logfilepath+"/users/"+traveler_name+".log");
		std::ofstream splist;
		if (args->splitregionpath != "") splist.open(args->splitregionpath+"/list_files/"+travname);
		time_t StartTime = time(0);
		log << "Log file created at: " << ctime(&StartTime);
		std::vector<char*> lines;
		std::vector<std::string> endlines;
		std::ifstream file(args->userlistfilepath+"/"+travname);
		// we can't getline here because it only allows one delimiter, and we need two; '\r' and '\n'.
		// at least one .list file contains newlines using only '\r' (0x0D):
		// https://github.com/TravelMapping/UserData/blob/6309036c44102eb3325d49515b32c5eef3b3cb1e/list_files/whopperman.list
		file.seekg(0, std::ios::end);
		unsigned long listdatasize = file.tellg();
		file.seekg(0, std::ios::beg);
		char *listdata = new char[listdatasize+1];
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
		size_t spn = 0;
		for (char *c = listdata; *c; c += spn)
		{	endlines.push_back("");
			spn = strcspn(c, "\r\n");
			while (c[spn] == '\r' || c[spn] == '\n')
			{	endlines.back().push_back(c[spn]);
				c[spn] = 0;
				spn++;
			}
			lines.push_back(c);
		}
		lines.push_back(listdata+listdatasize+1); // add a dummy "past-the-end" element to make lines[l+1]-2 work

		for (unsigned int l = 0; l < lines.size()-1; l++)
		{	std::string orig_line(lines[l]);
			// strip whitespace
			while (lines[l][0] == ' ' || lines[l][0] == '\t') lines[l]++;
			char * endchar = lines[l+1]-2; // -2 skips over the 0 inserted by strtok
			if (*endchar == 0) endchar--;  // skip back one more for CRLF cases FIXME what about lines followed by blank lines?
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
			strtok_mtx->lock();
			for (char *token = strtok(lines[l], " *+"); token; token = strtok(0, " *+") ) fields.push_back(token);
			strtok_mtx->unlock();
			if (fields.size() != 4)
			  // OK if 5th field exists and starts with #
			  if (fields.size() < 5 || fields[4][0] != '#')
			  {	bool invalid_char = 0;
				for (size_t c = 0; c < trim_line.size(); c++)
				  if (trim_line[c] < 0x20 || trim_line[c] >= 0x7F)
				  {	trim_line[c] = '?';
					invalid_char = 1;
				  }
				log << "Incorrect format line: " << trim_line;
				if (invalid_char) log << " [line contains invalid character(s)]";
				log << '\n';
				splist << orig_line << endlines[l];
				continue;
			  }

			// find the root that matches in some system and when we do, match labels
			std::string route_entry = lower(std::string(fields[1]));
			std::string lookup = std::string(lower(fields[0])) + " " + route_entry;
			try {	Route *r = route_hash->at(lookup);
				for (std:: string a : r->alt_route_names)
				  if (route_entry == lower(a))
				  {	log << "Note: deprecated route name " << fields[1]
					    << " -> canonical name " << r->list_entry_name() << " in line " << trim_line << '\n';
					break;
				  }
				if (r->system->devel())
				{	log << "Ignoring line matching highway in system in development: " << trim_line << '\n';
					splist << orig_line << endlines[l];
					continue;
				}
				// r is a route match, r.root is our root, and we need to find
				// canonical waypoint labels, ignoring case and leading
				// "+" or "*" when matching
				std::vector<unsigned int> point_indices;
				unsigned int checking_index = 0;
				for (Waypoint *w : r->point_list)
				{	std::string lower_label = lower(w->label);
					lower(fields[2]);
					lower(fields[3]);
					while (lower_label.front() == '+' || lower_label.front() == '*') lower_label.erase(lower_label.begin());
					if (fields[2] == lower_label || fields[3] == lower_label)
					     {	point_indices.push_back(checking_index);
						r->liu_mtx.lock();
						r->labels_in_use.insert(upper(lower_label));
						r->liu_mtx.unlock();
					     }
					else {	for (std::string &alt : w->alt_labels)
						{	lower_label = lower(alt);
							while (lower_label.front() == '+') lower_label.erase(lower_label.begin());
							if (fields[2] == lower_label || fields[3] == lower_label)
							{	point_indices.push_back(checking_index);
								r->liu_mtx.lock();
								r->labels_in_use.insert(upper(lower_label));
								r->liu_mtx.unlock();
								// if we have not yet used this alt label, remove it from the unused set
								r->ual_mtx.lock();
								r->unused_alt_labels.erase(upper(lower_label));
								r->ual_mtx.unlock();
							}
						}
					     }
					checking_index++;
				}
				if (point_indices.size() != 2)
				{	bool invalid_char = 0;
					for (size_t c = 0; c < trim_line.size(); c++)
					  if (trim_line[c] < 0x20 || trim_line[c] >= 0x7F)
					  {	trim_line[c] = '?';
						invalid_char = 1;
					  }
					log << "Waypoint label(s) not found in line: " << trim_line;
					if (invalid_char) log << " [line contains invalid character(s)]";
					log << '\n';
					splist << orig_line << endlines[l];
				}
				else {	list_entries++;
					// find the segments we just matched and store this traveler with the
					// segments and the segments with the traveler (might not need both
					// ultimately)
					for (unsigned int wp_pos = point_indices[0]; wp_pos < point_indices[1]; wp_pos++)
					{	HighwaySegment *hs = r->segment_list[wp_pos];
						hs->add_clinched_by(this);
						clinched_segments.insert(hs);
					}
					#include "splitregion.cpp"
				     }
			    }
			catch (const std::out_of_range& oor)
			    {	bool invalid_char = 0;
				for (size_t c = 0; c < trim_line.size(); c++)
				  if (trim_line[c] < 0x20 || trim_line[c] >= 0x7F)
				  {	trim_line[c] = '?';
					invalid_char = 1;
				  }
				log << "Unknown region/highway combo in line: " << trim_line;
				if (invalid_char) log << " [line contains invalid character(s)]";
				log << '\n';
				splist << orig_line << endlines[l];
			    }
		}
		delete[] listdata;
		log << "Processed " << list_entries << " good lines marking " << clinched_segments.size() << " segments traveled.\n";
		log.close();
		splist.close();
	}

	/* Return active mileage across all regions */
	double active_only_miles()
	{	double mi = 0;
		for (std::pair<Region*, double> rm : active_only_mileage_by_region) mi += rm.second;
		return mi;
	}

	/* Return active+preview mileage across all regions */
	double active_preview_miles()
	{	double mi = 0;
		for (std::pair<Region*, double> rm : active_preview_mileage_by_region) mi += rm.second;
		return mi;
	}

	/* Return mileage across all regions for a specified system */
	double system_region_miles(HighwaySystem *h)
	{	double mi = 0;
		for (std::pair<Region*, double> rm : system_region_mileages.at(h)) mi += rm.second;
		return mi;
	}

	#include "userlog.cpp"
};

std::mutex TravelerList::alltrav_mtx;

bool sort_travelers_by_name(const TravelerList *t1, const TravelerList *t2)
{	return t1->traveler_name < t2->traveler_name;
}
