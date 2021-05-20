// straightforward concurrency example with matching waypoint
// labels, use route/route/route@label, except also matches
// any hidden label

// TODO: compress when some but not all labels match, such as
//	 E127@Kan&AH60@Kan_N&AH64@Kan&AH67@Kan&M38@Kan
//	 ME9@PondRd&ME137@PondRd_Alb&US202@PondRd
//	 or possibly just compress ignoring the _ suffixes here
// TODO: US83@FM1263_S&US380@FM1263
//	 should probably end up as US83/US380@FM1263 or @FM1263_S
//  TRY: Use _suffix if all are the same; else drop
// TODO: ME11@I-95(109)&ME17@I-95&ME100@I-95(109)&US202@I-95(109)
//	 I-95 doesn't have a point here, due to a double trumpet.
//  TRY: Use (suffix) if all are the same; else drop

std::list<std::string> routes;
unsigned int matches = 0;
for (Waypoint *w : ap_coloc)
  if (ap_coloc.front()->label == w->label || w->label[0] == '+')
  {	// avoid double route names at border crossings
	if (!contains(routes, w->route->list_entry_name()))
		routes.push_back(w->route->list_entry_name());
	matches++;
  }
  else break;
if (matches == ap_coloc.size())
{	std::string newname;
	for (std::string &r : routes)
		newname += '/' + r;
	newname = newname.data()+1;
	newname += '@' + ap_coloc.front()->label;
	g->namelog("Straightforward_concurrency: " + name + " -> " + newname);
	return newname;
}
