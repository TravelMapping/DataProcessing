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
			else if (ap_coloc[check_index]->label.find(ap_coloc[other_index]->route->name_no_abbrev()) == 0)
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
		log.push_back("3+_intersection: " + name + " -> " + newname);
		return newname;
	}
}
