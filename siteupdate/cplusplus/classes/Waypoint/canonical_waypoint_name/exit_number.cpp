// Exit number simplification: I-90@47B(94)&I-94@47B
// becomes I-90/I-94(47B), with many other cases also matched

// TODO: I-20@76&I-77@16
//	 should become I-20/I-77 or maybe I-20(76)/I-77(16)
//	 not shorter, so maybe who cares about this one?
// TODO: HutRivPkwy@1&I-95@6B&I-278@I-95&I-295@I-95&I-678@19
// TODO: I-39@171C(90)&I-90@171C&US14@I-39/90
//	 I-90@134&I-93@16&MA3@16(93)&US1@I-93(16)
//	 I-93@12&MA3@12(93)&MA3APly@MA3_N&MA203@I-93&US1@I-93(12)
//	 I-95@182&I-395@1&ME15@I-95/395
// TODO: I-610@TX288&I-610@38&TX288@I-610
//	 this is the overlap point of a loop
// TODO: I-581@4&US220@I-581(4)&US460@I-581&US11AltRoa@I-581&US220AltRoa@US220_S&VA116@I-581(4)

// try each as a possible route@exit type situation and look
// for matches
for (Waypoint* exit : ap_coloc)
{	// see if all colocated points are potential matches
	// when considering the one at exit as a primary
	// exit number
	if (exit->label[0] < '0' || exit->label[0] > '9') continue;
	std::string no_abbrev = exit->route->name_no_abbrev();
	std::string nmbr_only = no_abbrev; // route number only version
	for (unsigned int pos = 0; pos < nmbr_only.size(); pos++)
	  if (nmbr_only[pos] >= '0' && nmbr_only[pos] <= '9')
	  {	nmbr_only = nmbr_only.data()+pos;
		break;
	  }
	size_t list_name_size = no_abbrev.size()+exit->route->abbrev.size();

	bool all_match = 1;
	for (Waypoint* match : ap_coloc)
	{	if (exit == match) continue;
		// check for any of the patterns that make sense as a match:

		// if name_no_abbrev() matches, check for...
		if ( !strncmp(match->label.data(),
			      no_abbrev.data(),
			      no_abbrev.size())
		   ) {	if (	match->label[no_abbrev.size()] == 0	// full match without abbrev field
			     || match->label[no_abbrev.size()] == '_'	// match with _ suffix (like _N)
			     || match->label[no_abbrev.size()] == '/'	// match with a slash
			   )	continue;
			if (	match->label[no_abbrev.size()] == '('
			     && !strncmp(match->label.data()+no_abbrev.size()+1,
					 exit->label.data(),
					 exit->label.size())
			     && match->label[no_abbrev.size()+exit->label.size()+1] == ')'
			   )	continue;				// match with exit number in parens

			// if abbrev matches, check for...
			if ( !strncmp(match->label.data()+no_abbrev.size(),
				      exit->route->abbrev.data(),
				      exit->route->abbrev.size())
			   ) {	if (	match->label[list_name_size] == 0	// full match with abbrev field
				     || match->label[list_name_size] == '_'	// match with _ suffix (like _N)
				     || match->label[list_name_size] == '/'	// match with a slash
				   )	continue;
				if (	match->label[list_name_size] == '('
				     && !strncmp(match->label.data()+list_name_size+1,
						 exit->label.data(),
						 exit->label.size())
				     && match->label[list_name_size+exit->label.size()+1] == ')'
				   )	continue;				// match with exit number in parens
			     }
		     }

		if (match->label != exit->label				 // match with exit number only
		 && match->label != exit->label + '(' + nmbr_only + ')'	 // match concurrency exit
		 && match->label != exit->label + '(' + no_abbrev + ')') // number format nn(rr)
		{	all_match = 0;
			break;
		}
	}
	if (all_match)
	{	std::string newname(exit->route->list_entry_name() + '(' + exit->label + ')');
		for (unsigned int pos = 0; pos < ap_coloc.size(); pos++)
		{	if (ap_coloc[pos] != exit)
				newname += '/' + ap_coloc[pos]->route->list_entry_name();
		}
		g->namelog("Exit_number: " + name + " -> " + newname);
		return newname;
	}
}
