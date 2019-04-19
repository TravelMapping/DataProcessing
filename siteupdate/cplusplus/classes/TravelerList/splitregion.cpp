// new .list lines for region split-ups
if (args->splitregion == upper(fields[0]))
{	// first, comment out original line
	splist << "##### " << orig_line << newline;
	HighwaySegment *orig_hs = r->segment_list[canonical_waypoint_indices[0]];
	HighwaySegment *new_hs = 0;
	if (!orig_hs->concurrent)
		std::cout << "ERROR: " << orig_hs->str() << " not concurrent" << std::endl;
	else {	size_t count = 0;
		// find concurrent segment with same name in different region
		for (HighwaySegment *cs : *(orig_hs->concurrent))
		  if (cs->route->name_no_abbrev() == r->name_no_abbrev() && cs->route->region != r->region)
		  {	count++;
			new_hs = cs;
		  }
		if (!new_hs) std::cout << "ERROR: concurrent segment not found for " << orig_hs->str() << std::endl;
		else {	if (count > 1) std::cout << "DEBUG: multiple matches found for " << orig_hs->str() << std::endl;
			// get lines from associated connected route.
			// assumption: each chopped route in old full region corresponds 1:1 to a connected route in new chopped regions
			splist << new_hs->route->con_route->list_lines(canonical_waypoint_indices[0],
								       canonical_waypoint_indices[1] -
								       canonical_waypoint_indices[0],
								       newline, 2) << endlines[l];
		     }
	     }
}
else splist << orig_line << endlines[l];
