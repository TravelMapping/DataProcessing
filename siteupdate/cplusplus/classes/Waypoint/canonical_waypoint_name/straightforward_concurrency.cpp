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
  else break;
if (matches == ap_coloc.size())
{	log.push_back("Straightforward concurrency: " + name + " -> " + routes + "@" + pointname);
	return routes + "@" + pointname;
}
