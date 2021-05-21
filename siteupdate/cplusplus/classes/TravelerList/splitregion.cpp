Waypoint *w1, *w2;
// comment out original line and indent new line below
splist << "##### " << orig_line << newline << "  ";

// 1st waypoint
if (Args::splitregion != r1->region->code)
	// if not in splitregion, just use canonical .list name and waypoint label
	w1 = r1->point_list[index1];
else {	HighwaySegment *orig_hs = r1->segment_list[index1 == r1->segment_list.size() ? index1-1 : index1];
	HighwaySegment *new_hs = 0;
	if (!orig_hs->concurrent)
		std::cout << "ERROR: " << orig_hs->str() << " not concurrent" << std::endl;
	else {	size_t count = 0;
		// find concurrent segment with same name in different region
		for (HighwaySegment *cs : *(orig_hs->concurrent))
		  if (cs->route->name_no_abbrev() == r1->name_no_abbrev() && cs->route->region != r1->region)
		  {	count++;
			new_hs = cs;
		  }
		if (!new_hs) std::cout << "ERROR: concurrent segment not found for " << orig_hs->str() << std::endl;
		else {	if (count > 1) std::cout << "DEBUG: multiple matches found for " << orig_hs->str() << std::endl;
			// assumption: each chopped route in old full region corresponds 1:1 to a connected route in new chopped regions
			// index within old chopped route = index within new connected route.
			// convert to index within new chopped route
			for (long int r = new_hs->route->rootOrder; r > 0; r--)
				index1 -= new_hs->route->con_route->roots[r-1]->segment_list.size();
			w1 = new_hs->route->point_list[index1];
		     }
	     }
     }

// 2nd waypoint
if (Args::splitregion != r2->region->code)
	// if not in splitregion, just use canonical .list name and waypoint label
	w2 = r2->point_list[index2];
else {	HighwaySegment *orig_hs = r2->segment_list[index2 ? index2-1 : 0];
	HighwaySegment *new_hs = 0;
	if (!orig_hs->concurrent)
		std::cout << "ERROR: " << orig_hs->str() << " not concurrent" << std::endl;
	else {	size_t count = 0;
		// find concurrent segment with same name in different region
		for (HighwaySegment *cs : *(orig_hs->concurrent))
		  if (cs->route->name_no_abbrev() == r2->name_no_abbrev() && cs->route->region != r2->region)
		  {	count++;
			new_hs = cs;
		  }
		if (!new_hs) std::cout << "ERROR: concurrent segment not found for " << orig_hs->str() << std::endl;
		else {	if (count > 1) std::cout << "DEBUG: multiple matches found for " << orig_hs->str() << std::endl;
			// assumption: each chopped route in old full region corresponds 1:1 to a connected route in new chopped regions
			// index within old chopped route = index within new connected route.
			// convert to index within new chopped route
			for (long int r = new_hs->route->rootOrder; r > 0; r--)
				index2 -= new_hs->route->con_route->roots[r-1]->segment_list.size();
			w2 = new_hs->route->point_list[index2];
		     }
	     }
     }

// write new line to file
if (w1->route == w2->route) // 4 fields possible
	if (index1 > index2 || reverse)
		splist << w2->route->readable_name() << ' ' << w2->label << ' ' << w1->label << endlines[l];
	else	splist << w1->route->readable_name() << ' ' << w1->label << ' ' << w2->label << endlines[l];
else	if (reverse)
		splist << w2->route->readable_name() << ' ' << w2->label << ' ' << w1->route->readable_name() << ' ' << w1->label << endlines[l];
	else	splist << w1->route->readable_name() << ' ' << w1->label << ' ' << w2->route->readable_name() << ' ' << w2->label << endlines[l];
