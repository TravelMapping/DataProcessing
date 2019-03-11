void Route::read_wpt
(	WaypointQuadtree *all_waypoints, ErrorList *el, std::string path, std::mutex *strtok_mtx,
	DatacheckEntryList *datacheckerrors, std::unordered_set<std::string> *all_wpt_files
)
{	/* read data into the Route's waypoint list from a .wpt file */
	//cout << "read_wpt on " << str() << endl;
	std::string filename = path + "/" + region->code + "/" + system->systemname + "/" + root + ".wpt";
	// remove full path from all_wpt_files list
	awf_mtx.lock();
	all_wpt_files->erase(filename);
	awf_mtx.unlock();
	std::vector<char*> lines;
	std::ifstream file(filename.data());
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
	strtok_mtx->lock();
	for (char *token = strtok(wptdata, "\r\n"); token; token = strtok(0, "\r\n") ) lines.emplace_back(token);
	strtok_mtx->unlock();
	// set to be used per-route to find label duplicates
	std::unordered_set<std::string> all_route_labels;
	// set to be used for finding duplicate coordinates
	std::unordered_set<Waypoint*> coords_used;
	double vis_dist = 0;
	Waypoint *last_visible = 0;
	char fstr[112];

	for (char *line : lines)
	{	//FIXME incorporate fix for whitespace-only lines
		if (line[0] == 0) continue;
		Waypoint *w = new Waypoint(line, this, strtok_mtx, datacheckerrors);
		point_list.push_back(w);
		// populate unused alt labels
		for (size_t i = 0; i < w->alt_labels.size(); i++)
		{	std::string al = w->alt_labels[i];
			// strip out leading '+'
			while (al[0] == '+') al.erase(0, 1);	//TODO would erase via iterator be any faster?
			// convert to upper case
			for (size_t c = 0; c < al.size(); c++)
			  if (al[c] >= 'a' && al[c] <= 'z') al[c] -= 32;
			unused_alt_labels.insert(al);
		}
		// look for colocated points
		all_waypoints->mtx.lock();
		Waypoint *other_w = all_waypoints->waypoint_at_same_point(w);
		if (other_w)
		{	// see if this is the first point colocated with other_w
			if (!other_w->colocated)
			{	other_w->colocated = new std::list<Waypoint*>;
				other_w->colocated->push_front(other_w);
			}
			other_w->colocated->push_front(w);
			w->colocated = other_w->colocated;
		}
		// look for near-miss points (before we add this one in)
		//cout << "DEBUG: START search for nmps for waypoint " << w->str() << " in quadtree of size " << all_waypoints.size() << endl;
		w->near_miss_points = all_waypoints->near_miss_waypoints(w, 0.0005);
		/*cout << "DEBUG: for waypoint " << w->str() << " got " << w->near_miss_points.size() << " nmps: ";
		for (Waypoint *dbg_w : w->near_miss_points)
			cout << dbg_w->str() << " ";
		cout << endl;//*/
		for (Waypoint *other_w : w->near_miss_points) other_w->near_miss_points.push_front(w);

		all_waypoints->insert(w);
		all_waypoints->mtx.unlock();

		// single-point Datachecks, and HighwaySegment
		w->out_of_bounds(datacheckerrors, fstr);
		w->duplicate_label(datacheckerrors, all_route_labels);
		w->duplicate_coords(datacheckerrors, coords_used, fstr);
		if (point_list.size() > 1)
		{	w->distance_update(datacheckerrors, fstr, vis_dist, point_list[point_list.size()-2]);
			// add HighwaySegment, if not first point
			segment_list.push_back(new HighwaySegment(point_list[point_list.size()-2], w, this));
		}
		// checks for visible points
		if (!w->is_hidden)
		{	w->visible_distance(datacheckerrors, fstr, vis_dist, last_visible);
			const char *slash = strchr(w->label.data(), '/');
			w->label_selfref(datacheckerrors, slash);
			w->label_slashes(datacheckerrors, slash);
			w->underscore_datachecks(datacheckerrors, slash);
			w->label_parens(datacheckerrors);
			w->label_invalid_char(datacheckerrors, w->label);
			  for (std::string &a : w->alt_labels) w->label_invalid_char(datacheckerrors, a);
			w->bus_with_i(datacheckerrors);
			w->label_looks_hidden(datacheckerrors);
		}
	}

	// per-route datachecks

	// look for hidden termini
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

	if (point_list.size() < 2) el->add_error("Route contains fewer than 2 points: " + str());
	std::cout << '.' << std::flush;
	//std::cout << str() << std::flush;
	//print_route();
}