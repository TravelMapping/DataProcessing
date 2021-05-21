// 3+ intersection with matching or partially matching labels
// NY5@NY16/384&NY16@NY5/384&NY384@NY5/16
// becomes NY5/NY16/NY384
// or a more complex case:
// US1@US21/176&US21@US1/378&US176@US1/378&US321@US1/378&US378@US21/176
// becomes US1/US21/US176/US321/US378

// TODO: N493@RingSpi&RingSpi@N493_E&RingSpi@N493_W -> N493_W/RingSpi/RingSpi

// approach: check if each label starts with some route number
// in the list of colocated routes, and if so, create a label
// slashing together all of the route names, and save any _
// suffixes to put in and reduce the chance of conflicting names
// and a second check to find matches when labels do not include
// the abbrev field (which they often do not)
if (ap_coloc.size() > 2)
{	bool match;
	std::vector<std::string> suffixes(ap_coloc.size());
	for (unsigned int check = 0; check < ap_coloc.size(); check++)
	{	match = 0;
		for (unsigned int other = 0; other < ap_coloc.size(); other++)
		{	if (other == check) continue;
			std::string other_no_abbrev = ap_coloc[other]->route->name_no_abbrev();
			if ( !strncmp(ap_coloc[check]->label.data(), other_no_abbrev.data(), other_no_abbrev.size()) )
			{	// should check here for false matches, like
				// NY50/67 would match startswith NY5
				match = 1;
				const char* suffix = strchr(ap_coloc[check]->label.data(), '_');
				if (suffix
				// we confirmed above that other_no_abbrev matches, so skip past it
				// we need only match the suffix with or without the abbrev
				 && (!strcmp(ap_coloc[check]->label.data()+other_no_abbrev.size(), suffix)
				  || ap_coloc[check]->label.data()+other_no_abbrev.size() == ap_coloc[other]->route->abbrev + suffix
				   ))	suffixes[other] = suffix;
			}
		}
		if (!match) break;
	}
	if (match)
	{	std::string newname = ap_coloc[0]->route->list_entry_name() + suffixes[0];
		for (unsigned int index = 1; index < ap_coloc.size(); index++)
			newname += '/' + ap_coloc[index]->route->list_entry_name() + suffixes[index];
		g->namelog("3+_intersection: " + name + " -> " + newname);
		return newname;
	}
}
