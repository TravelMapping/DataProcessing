// straightforward concurrency example with matching waypoint
// labels, use route/route/route@label, except also matches
// any hidden label
// TODO: compress when some but not all labels match, such as
// E127@Kan&AH60@Kan_N&AH64@Kan&AH67@Kan&M38@Kan
// or possibly just compress ignoring the _ suffixes here
std::list<std::string> routes;
unsigned int matches = 0;
for (Waypoint *w : ap_coloc)
  if (ap_coloc.front()->label == w->label || w->label[0] == '+')
  {	// avoid double route names at border crossings
	if (!list_contains(&routes, w->route->list_entry_name()))
		routes.push_back(w->route->list_entry_name());
	matches++;
  }
  else break;
if (matches == ap_coloc.size())
{	std::string newname;
	for (std::string &r : routes)
		newname += '/' + r;
	newname += '@' + ap_coloc.front()->label;
	log.push_back("Straightforward_concurrency: " + name + " -> " + newname.substr(1));
	return newname.substr(1);
}
