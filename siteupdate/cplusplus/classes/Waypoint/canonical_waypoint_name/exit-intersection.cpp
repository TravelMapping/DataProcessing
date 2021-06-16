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

for (Waypoint* match : ap_coloc)
{	std::string no_abbrev = match->route->name_no_abbrev();
	size_t list_name_size = no_abbrev.size()+match->route->abbrev.size();
	bool all_match = 1;
	for (Waypoint* check : ap_coloc)
	{	if (check == match) continue;

		// if name_no_abbrev() matches, check for...
		if ( !strncmp(check->label.data(),
			      no_abbrev.data(),
			      no_abbrev.size())
		   ) {	if (	check->label[no_abbrev.size()] == 0	// no_abbrev match
			   )	continue;
			if (	check->label[no_abbrev.size()] == '('
			     && !strncmp(check->label.data()+no_abbrev.size()+1,
					 match->label.data(),
					 match->label.size())
			     && check->label[no_abbrev.size()+match->label.size()+1] == ')'
			   )	continue;				// match with exit number in parens

			// if abbrev matches, check for...
			if ( !strncmp(check->label.data()+no_abbrev.size(),
				      match->route->abbrev.data(),
				      match->route->abbrev.size())
			   ) {	if (	check->label[list_name_size] == 0	// full match with abbrev field
				   )	continue;
				if (	check->label[list_name_size] == '('
				     && !strncmp(check->label.data()+list_name_size+1,
						 match->label.data(),
						 match->label.size())
				     && check->label[list_name_size+match->label.size()+1] == ')'
				   )	continue;				// match with exit number in parens
			     }
		     }
		all_match = 0;
		break;
	}
	if (all_match)
	{	std::string newname(match->route->list_entry_name());
		if (match->label[0] >= '0' && match->label[0] <= '9')
			newname +=  '(' + match->label + ')';
		for (unsigned int add_index = 0; add_index < ap_coloc.size(); add_index++)
		{	if (match == ap_coloc[add_index]
			 || (add_index && ap_coloc[add_index]->route == ap_coloc[add_index-1]->route)
			   ) continue;
			newname += '/' + ap_coloc[add_index]->route->list_entry_name();
		}
		g->namelog("Exit/Intersection: " + name + " -> " + newname);
		return newname;
	}
}
