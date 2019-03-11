std::string Waypoint::canonical_waypoint_name(std::list<std::string> &log)
{	/* Best name we can come up with for this point bringing in
	information from itself and colocated points (if active/preview)
	*/
	// start with the failsafe name, and see if we can improve before
	// returning
	std::string name = simple_waypoint_name();

	// if no colocated points, there's nothing to do - we just use
	// the route@label form and deal with conflicts elsewhere
	if (!colocated) return name;

	// get a colocated list with any devel system entries removed
	std::vector<Waypoint *> ap_coloc;
	for (Waypoint *w : *colocated)
	  if (w->route->system->active_or_preview())
	    ap_coloc.push_back(w);
	// just return the simple name if only one active/preview waypoint
	if (ap_coloc.size() == 1) return name;

	// straightforward concurrency example with matching waypoint
	// labels, use route/route/route@label, except also matches
	// any hidden label
	// TODO: compress when some but not all labels match, such as
	// E127@Kan&AH60@Kan_N&AH64@Kan&AH67@Kan&M38@Kan
	// or possibly just compress ignoring the _ suffixes here
	std::string routes, pointname;
	unsigned int matches = 0;
	for (Waypoint *w : ap_coloc)
	  if (routes == "")
	  {	routes = w->route->list_entry_name();
		pointname = w->label;
		matches = 1;
	  }
	  else if (pointname == w->label || w->label[0] == '+')
	  {	// this check seems odd, but avoids double route names
		// at border crossings
		if (routes != w->route->list_entry_name())
			routes += "/" + w->route->list_entry_name();
		matches++;
	  }
	if (matches == ap_coloc.size())
	{	log.push_back("Straightforward concurrency: " + name + " -> " + routes + "@" + pointname);
		return routes + "@" + pointname;
	}

	// straightforward 2-route intersection with matching labels
	// NY30@US20&US20@NY30 would become NY30/US20
	// or
	// 2-route intersection with one or both labels having directional
	// suffixes but otherwise matching route
	// US24@CO21_N&CO21@US24_E would become US24_E/CO21_N

	if (ap_coloc.size() == 2)
	{	std::string w0_list_entry = ap_coloc[0]->route->list_entry_name();
		std::string w1_list_entry = ap_coloc[1]->route->list_entry_name();
		std::string w0_label = ap_coloc[0]->label;
		std::string w1_label = ap_coloc[1]->label;
		if (	(w0_list_entry == w1_label or w1_label.find(w0_list_entry + '_') == 0)
		     &&	(w1_list_entry == w0_label or w0_label.find(w1_list_entry + '_') == 0)
		   )
		   {	log.push_back("Straightforward intersection: " + name + " -> " + w1_label + "/" + w0_label);
			return w1_label + "/" + w0_label;
		   }
	}

	// check for cases like
	// I-10@753B&US90@I-10(753B)
	// which becomes
	// I-10(753B)/US90
	// more generally,
	// I-30@135&US67@I-30(135)&US70@I-30(135)
	// becomes
	// I-30(135)/US67/US70
	// but also matches some other cases that perhaps should
	// be checked or handled separately, though seems OK
	// US20@NY30A&NY30A@US20&NY162@US20
	// becomes
	// US20/NY30A/NY162

	for (unsigned int match_index = 0; match_index < ap_coloc.size(); match_index++)
	{	std::string lookfor1 = ap_coloc[match_index]->route->list_entry_name();
		std::string lookfor2 = ap_coloc[match_index]->route->list_entry_name() + "(" + ap_coloc[match_index]->label + ")";
		bool all_match = 1;
		for (unsigned int check_index = 0; check_index < ap_coloc.size(); check_index++)
		{	if (match_index == check_index) continue;
			if ((ap_coloc[check_index]->label != lookfor1)
			&&  (ap_coloc[check_index]->label != lookfor2))
				all_match = 0;
		}
		if (all_match)
		{	std::string newname;
			if (ap_coloc[match_index]->label[0] >= '0' && ap_coloc[match_index]->label[0] <= '9')
				newname = lookfor2;
			else	newname = lookfor1;
			for (unsigned int add_index = 0; add_index < ap_coloc.size(); add_index++)
			{	if (match_index == add_index) continue;
				newname += "/" + ap_coloc[add_index]->route->list_entry_name();
			}
			log.push_back("Exit/Intersection: " + name + " -> " + newname);
			return newname;
		}
	}

	// TODO: NY5@NY16/384&NY16@NY5/384&NY384@NY5/16
	// should become NY5/NY16/NY384
		
	// 3+ intersection with matching or partially matching labels
	// NY5@NY16/384&NY16@NY5/384&NY384@NY5/16
	// becomes NY5/NY16/NY384

	// or a more complex case:
	// US1@US21/176&US21@US1/378&US176@US1/378&US321@US1/378&US378@US21/176
	// becomes US1/US21/US176/US321/US378
	// approach: check if each label starts with some route number
	// in the list of colocated routes, and if so, create a label
	// slashing together all of the route names, and save any _
	// suffixes to put in and reduce the chance of conflicting names
	// and a second check to find matches when labels do not include
	// the abbrev field (which they often do not)
	if (ap_coloc.size() > 2)
	{	bool all_match = 1;
		std::vector<std::string> suffixes(ap_coloc.size());
		for (unsigned int check_index = 0; check_index < ap_coloc.size(); check_index++)
		{	bool this_match = 0;
			for (unsigned int other_index = 0; other_index < ap_coloc.size(); other_index++)
			{	if (other_index == check_index) continue;
				if (ap_coloc[check_index]->label.find(ap_coloc[other_index]->route->list_entry_name()) == 0)
				{	// should check here for false matches, like
					// NY50/67 would match startswith NY5
					this_match = 1;
					if (strchr(ap_coloc[check_index]->label.data(), '_'))
					{	std::string suffix = strchr(ap_coloc[check_index]->label.data(), '_');
						if (ap_coloc[other_index]->route->list_entry_name() + suffix == ap_coloc[check_index]->label)
							suffixes[other_index] = suffix;
					}
				}
				else if (ap_coloc[check_index]->label.find(ap_coloc[other_index]->route->name_no_abbrev()) == 0) //FIXME -> elif in siteupdate.py
				{	this_match = 1;
					if (strchr(ap_coloc[check_index]->label.data(), '_'))
					{	std::string suffix = strchr(ap_coloc[check_index]->label.data(), '_');
						if (ap_coloc[other_index]->route->name_no_abbrev() + suffix == ap_coloc[check_index]->label)
							suffixes[other_index] = suffix;
					}
				}
			}
			if (!this_match)
			{	all_match = 0;
				break;
			}
		}
		if (all_match)
		{	std::string newname = ap_coloc[0]->route->list_entry_name() + suffixes[0];
			for (unsigned int index = 1; index < ap_coloc.size(); index++)
				newname += "/" + ap_coloc[index]->route->list_entry_name() + suffixes[index];
			log.push_back("3+ intersection: " + name + " -> " + newname);
			return newname;
		}
	}

	// Exit number simplification: I-90@47B(94)&I-94@47B
	// becomes I-90/I-94@47B, with many other cases also matched
	// Still TODO: I-39@171C(90)&I-90@171C&US14@I-39/90
	// try each as a possible route@exit type situation and look
	// for matches
	for (unsigned int try_as_exit = 0; try_as_exit < ap_coloc.size(); try_as_exit++)
	{	// see if all colocated points are potential matches
		// when considering the one at try_as_exit as a primary
		// exit number
		if (ap_coloc[try_as_exit]->label[0] < '0' || ap_coloc[try_as_exit]->label[0] > '9') continue;
		bool all_match = 1;
		// get the route number only version for one of the checks below
		std::string route_number_only = ap_coloc[try_as_exit]->route->name_no_abbrev();
		for (unsigned int pos = 0; pos < route_number_only.size(); pos++)
		  if (route_number_only[pos] >= '0' && route_number_only[pos] <= '9')
		  {	route_number_only = route_number_only.substr(pos);
			break;
		  }
		for (unsigned int try_as_match = 0; try_as_match < ap_coloc.size(); try_as_match++)
		{	if (try_as_exit == try_as_match) continue;
			bool this_match = 0;
			// check for any of the patterns that make sense as a match:
			// exact match, match without abbrev field, match with exit
			// number in parens, match concurrency exit number format
			// nn(rr), match with _ suffix (like _N), match with a slash
			// match with exit number only
			if (ap_coloc[try_as_match]->label == ap_coloc[try_as_exit]->route->list_entry_name()
			 || ap_coloc[try_as_match]->label == ap_coloc[try_as_exit]->route->name_no_abbrev()
			 || ap_coloc[try_as_match]->label == ap_coloc[try_as_exit]->route->list_entry_name() + "(" + ap_coloc[try_as_exit]->label + ")"
			 || ap_coloc[try_as_match]->label == ap_coloc[try_as_exit]->label + "(" + route_number_only + ")"
			 || ap_coloc[try_as_match]->label == ap_coloc[try_as_exit]->label + "(" + ap_coloc[try_as_exit]->route->name_no_abbrev() + ")"
			 || ap_coloc[try_as_match]->label.find(ap_coloc[try_as_exit]->route->name_no_abbrev() + "_") == 0
			 || ap_coloc[try_as_match]->label.find(ap_coloc[try_as_exit]->route->name_no_abbrev() + "/") == 0
			 || ap_coloc[try_as_match]->label == ap_coloc[try_as_exit]->label)
				this_match = 1;
			if (!this_match) all_match = 0;
		}
		if (all_match)
		{	std::string newname = "";
			for (unsigned int pos = 0; pos < ap_coloc.size(); pos++)
			{	if (pos == try_as_exit)
					newname += ap_coloc[pos]->route->list_entry_name() + "(" + ap_coloc[pos]->label + ")";
				else	newname += ap_coloc[pos]->route->list_entry_name();
				if (pos < ap_coloc.size()-1)
					newname += "/";
			}
			log.push_back("Exit number: " + name + " -> " + newname);
			return newname;
		}
	}

	// TODO: I-20@76&I-77@16
	// should become I-20/I-77 or maybe I-20(76)/I-77(16)
	// not shorter, so maybe who cares about this one?

	// TODO: US83@FM1263_S&US380@FM1263
	// should probably end up as US83/US280@FM1263 or @FM1263_S

	// How about?
	// I-581@4&US220@I-581(4)&US460@I-581&US11AltRoa@I-581&US220AltRoa@US220_S&VA116@I-581(4)
	// INVESTIGATE: VA262@US11&US11@VA262&VA262@US11_S
	// should be 2 colocated, shows up as 3?

	// TODO: I-610@TX288&I-610@38&TX288@I-610
	// this is the overlap point of a loop

	// TODO: boundaries where order is reversed on colocated points
	// Vt4@FIN/NOR&E75@NOR/FIN&E75@NOR/FIN

	log.push_back("Keep failsafe: " + name);
	return name;
}
