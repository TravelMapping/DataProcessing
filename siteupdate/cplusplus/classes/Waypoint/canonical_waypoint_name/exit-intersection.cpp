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
	std::string lookfor2 = ap_coloc[match_index]->route->list_entry_name() + '(' + ap_coloc[match_index]->label + ')';
	bool all_match = 1;
	for (unsigned int check_index = 0; check_index < ap_coloc.size(); check_index++)
	{	if (match_index == check_index) continue;
		if ((ap_coloc[check_index]->label != lookfor1)
		&&  (ap_coloc[check_index]->label != lookfor2))
		{	all_match = 0;
			break;
		}
	}
	if (all_match)
	{	std::string newname;
		if (ap_coloc[match_index]->label[0] >= '0' && ap_coloc[match_index]->label[0] <= '9')
			newname = lookfor2;
		else	newname = lookfor1;
		for (unsigned int add_index = 0; add_index < ap_coloc.size(); add_index++)
		{	if (match_index == add_index) continue;
			newname += '/' + ap_coloc[add_index]->route->list_entry_name();
		}
		g->namelog("Exit/Intersection: " + name + " -> " + newname);
		return newname;
	}
}
