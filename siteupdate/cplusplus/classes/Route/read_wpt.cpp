void Route::read_wpt
(	WaypointQuadtree *all_waypoints, ErrorList *el, std::string path,
	DatacheckEntryList *datacheckerrors, std::unordered_set<std::string> *all_wpt_files
)
{	/* read data into the Route's waypoint list from a .wpt file */
	//cout << "read_wpt on " << str() << endl;
	std::string filename = path + "/" + rg_str + "/" + system->systemname + "/" + root + ".wpt";
	// remove full path from all_wpt_files list
	awf_mtx.lock();
	all_wpt_files->erase(filename);
	awf_mtx.unlock();
	std::vector<char*> lines;
	std::ifstream file(filename);
	if (!file)
	{	el->add_error("Could not open " + filename);
		file.close();
		return;
	}
	file.seekg(0, std::ios::end);
	unsigned long wptdatasize = file.tellg();
	file.seekg(0, std::ios::beg);
	char *wptdata = new char[wptdatasize+1];
	file.read(wptdata, wptdatasize);
	wptdata[wptdatasize] = 0; // add null terminator
	file.close();

	// split file into lines
	size_t spn = 0;
	for (char* c = wptdata; *c; c += spn)
	{	for (spn = strcspn(c, "\n\r"); c[spn] == '\n' || c[spn] == '\r'; spn++) c[spn] = 0;
		lines.emplace_back(c);
	}

	lines.push_back(wptdata+wptdatasize+1); // add a dummy "past-the-end" element to make lines[l+1]-2 work
	// set to be used for finding duplicate coordinates
	std::unordered_set<Waypoint*> coords_used;
	double vis_dist = 0;
	Waypoint *last_visible = 0;
	char fstr[112];

	for (unsigned int l = 0; l < lines.size()-1; l++)
	{	// strip whitespace
		while (lines[l][0] == ' ' || lines[l][0] == '\t') lines[l]++;
		char * endchar = lines[l+1]-2; // -2 skips over the 0 inserted while splitting wptdata into lines
		while (*endchar == 0) endchar--;  // skip back more for CRLF cases, and lines followed by blank lines
		while (*endchar == ' ' || *endchar == '\t')
		{	*endchar = 0;
			endchar--;
		}
		if (lines[l][0] == 0) continue;
		Waypoint *w = new Waypoint(lines[l], this, datacheckerrors);
			      // deleted on termination of program, or immediately below if invalid
		bool malformed_url = w->lat == 0 && w->lng == 0;
		bool label_too_long = w->label_too_long(datacheckerrors);
		if (malformed_url || label_too_long)
		{	delete w;
			continue;
		}
		point_list.push_back(w);
		all_waypoints->insert(w, 1);

		// single-point Datachecks, and HighwaySegment
		w->out_of_bounds(datacheckerrors, fstr);
		w->duplicate_coords(datacheckerrors, coords_used, fstr);
		w->label_invalid_char(datacheckerrors);
		if (point_list.size() > 1)
		{	w->distance_update(datacheckerrors, fstr, vis_dist, point_list[point_list.size()-2]);
			// add HighwaySegment, if not first point
			segment_list.push_back(new HighwaySegment(point_list[point_list.size()-2], w, this));
					       // deleted on termination of program
		}
		// checks for visible points
		if (!w->is_hidden)
		{	w->visible_distance(datacheckerrors, fstr, vis_dist, last_visible);
			const char *slash = strchr(w->label.data(), '/');
			w->label_selfref(datacheckerrors, slash);
			w->label_slashes(datacheckerrors, slash);
			w->underscore_datachecks(datacheckerrors, slash);
			w->label_parens(datacheckerrors);
			w->label_invalid_ends(datacheckerrors);
			w->bus_with_i(datacheckerrors);
			w->label_looks_hidden(datacheckerrors);
			w->lacks_generic(datacheckerrors);
		}
	}
	delete[] wptdata;

	// per-route datachecks
	if (point_list.size() < 2) el->add_error("Route contains fewer than 2 points: " + str());
	else {	// look for hidden termini
		if (point_list.front()->is_hidden)	datacheckerrors->add(this, point_list.front()->label, "", "", "HIDDEN_TERMINUS", "");
		if (point_list.back()->is_hidden)	datacheckerrors->add(this, point_list.back()->label, "", "", "HIDDEN_TERMINUS", "");

		// angle check is easier with a traditional for loop and array indices
		for (unsigned int i = 1; i < point_list.size()-1; i++)
		{	//cout << "computing angle for " << point_list[i-1].str() << ' ' << point_list[i].str() << ' ' << point_list[i+1].str() << endl;
			if (point_list[i-1]->same_coords(point_list[i]) || point_list[i+1]->same_coords(point_list[i]))
				datacheckerrors->add(this, point_list[i-1]->label, point_list[i]->label, point_list[i+1]->label, "BAD_ANGLE", "");
			else {	double angle = point_list[i]->angle(point_list[i-1], point_list[i+1]);
				if (angle > 135)
				{	sprintf(fstr, "%.2f", angle);
					datacheckerrors->add(this, point_list[i-1]->label, point_list[i]->label, point_list[i+1]->label, "SHARP_ANGLE", fstr);
				}
			     }
		}
	     }
	std::cout << '.' << std::flush;
	//std::cout << str() << std::flush;
	//print_route();
}
