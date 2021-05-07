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
		if (ap_coloc[try_as_match]->label == ap_coloc[try_as_exit]->route->name_no_abbrev()
		 || ap_coloc[try_as_match]->label == ap_coloc[try_as_exit]->route->name_no_abbrev() + '(' + ap_coloc[try_as_exit]->label + ')'
		 || ap_coloc[try_as_match]->label == ap_coloc[try_as_exit]->route->list_entry_name()
		 || ap_coloc[try_as_match]->label == ap_coloc[try_as_exit]->route->list_entry_name() + '(' + ap_coloc[try_as_exit]->label + ')'
		 || ap_coloc[try_as_match]->label == ap_coloc[try_as_exit]->label
		 || ap_coloc[try_as_match]->label == ap_coloc[try_as_exit]->label + '(' + route_number_only + ')'
		 || ap_coloc[try_as_match]->label == ap_coloc[try_as_exit]->label + '(' + ap_coloc[try_as_exit]->route->name_no_abbrev() + ')'
		 || ap_coloc[try_as_match]->label.find(ap_coloc[try_as_exit]->route->name_no_abbrev() + '_') == 0
		 || ap_coloc[try_as_match]->label.find(ap_coloc[try_as_exit]->route->name_no_abbrev() + '/') == 0)
			this_match = 1;
		if (!this_match) all_match = 0;
	}
	if (all_match)
	{	std::string newname;
		for (unsigned int pos = 0; pos < ap_coloc.size(); pos++)
		{	if (pos == try_as_exit)
				newname += ap_coloc[pos]->route->list_entry_name() + '(' + ap_coloc[pos]->label + ')';
			else	newname += ap_coloc[pos]->route->list_entry_name();
			if (pos < ap_coloc.size()-1)
				newname += '/';
		}
		log.push_back("Exit_number: " + name + " -> " + newname);
		return newname;
	}
}
