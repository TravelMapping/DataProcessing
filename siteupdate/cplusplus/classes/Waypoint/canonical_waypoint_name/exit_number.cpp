// Exit number simplification: I-90@47B(94)&I-94@47B
// becomes I-90/I-94@47B, with many other cases also matched
// Still TODO: I-39@171C(90)&I-90@171C&US14@I-39/90
// try each as a possible route@exit type situation and look
// for matches
for (Waypoint* exit : ap_coloc)
{	// see if all colocated points are potential matches
	// when considering the one at exit as a primary
	// exit number
	if (exit->label[0] < '0' || exit->label[0] > '9') continue;
	std::string list_name = exit->route->list_entry_name();
	std::string no_abbrev = exit->route->name_no_abbrev();
	std::string nmbr_only = no_abbrev; // route number only version
	for (unsigned int pos = 0; pos < nmbr_only.size(); pos++)
	  if (nmbr_only[pos] >= '0' && nmbr_only[pos] <= '9')
	  {	nmbr_only = nmbr_only.data()+pos;
		break;
	  }
	bool all_match = 1;
	for (Waypoint* match : ap_coloc)
	{	if (exit == match) continue;
		// check for any of the patterns that make sense as a match:
		// exact match, match without abbrev field, match with exit
		// number in parens, match concurrency exit number format
		// nn(rr), match with _ suffix (like _N), match with a slash
		// match with exit number only
		if (match->label != no_abbrev
		 && match->label != no_abbrev + '(' + exit->label + ')'
		 && match->label != list_name
		 && match->label != list_name + '(' + exit->label + ')'
		 && match->label != exit->label
		 && match->label != exit->label + '(' + nmbr_only + ')'
		 && match->label != exit->label + '(' + no_abbrev + ')'
		 && match->label.find(no_abbrev + '_') != 0
		 && match->label.find(no_abbrev + '/') != 0)
		{	all_match = 0;
			break;
		}
	}
	if (all_match)
	{	std::string newname;
		for (unsigned int pos = 0; pos < ap_coloc.size(); pos++)
		{	if (ap_coloc[pos] == exit)
				newname += ap_coloc[pos]->route->list_entry_name() + '(' + ap_coloc[pos]->label + ')';
			else	newname += ap_coloc[pos]->route->list_entry_name();
			newname += '/';
		}
		newname.pop_back();
		log.push_back("Exit_number: " + name + " -> " + newname);
		return newname;
	}
}
