std::string lookup1 = std::string(fields[0]) + ' ' + fields[1];
std::string lookup2 = std::string(fields[3]) + ' ' + fields[4];
upper(lookup1.data());
upper(lookup2.data());
// look for region/route combos, first in pri_list_hash
std::unordered_map<std::string,Route*>::iterator rit1 = Route::pri_list_hash.find(lookup1);
std::unordered_map<std::string,Route*>::iterator rit2 = Route::pri_list_hash.find(lookup2);
// and then if not found, in alt_list_hash
if (rit1 == Route::pri_list_hash.end())
{	rit1 = Route::alt_list_hash.find(lookup1);
	if (rit1 != Route::alt_list_hash.end())
		log << "Note: deprecated route name \"" << fields[0] << ' ' << fields[1]
		    << "\" -> canonical name \"" << rit1->second->readable_name() << "\" in line: " << trim_line << '\n';
}
if (rit2 == Route::pri_list_hash.end())
{	rit2 = Route::alt_list_hash.find(lookup2);
	if (rit2 != Route::alt_list_hash.end())
		log << "Note: deprecated route name \"" << fields[3] << ' ' << fields[4]
		    << "\" -> canonical name \"" << rit2->second->readable_name() << "\" in line: " << trim_line << '\n';
}
if (rit1 == Route::alt_list_hash.end() || rit2 == Route::alt_list_hash.end())
{	bool invalid_char = 0;
	for (char& c : trim_line)
	  if (iscntrl(c))
	  {	c = '?';
		invalid_char = 1;
	  }
	for (char& c : lookup1) if (iscntrl(c)) c = '?';
	for (char& c : lookup2) if (iscntrl(c)) c = '?';
	if (rit1 == rit2)
		log << "Unknown region/highway combos " << lookup1 << " and " << lookup2 << " in line: " << trim_line;
	else {	log << "Unknown region/highway combo ";
		log << (rit1 == Route::alt_list_hash.end() ? lookup1 : lookup2);
		log << " in line: " << trim_line;
	     }
	if (invalid_char) log << " [contains invalid character(s)]";
	log << '\n';
	splist << orig_line << endlines[l];
	continue;
}
Route* r1 = rit1->second;
Route* r2 = rit2->second;
if (r1->con_route != r2->con_route)
{	log << lookup1 << " and " << lookup2 << " not in same connected route in line: " << trim_line << '\n';
	splist << orig_line << endlines[l];
	UPDATE_NOTE(r1->con_route->roots.front()) if (r1->con_route->roots.size() > 1) UPDATE_NOTE(r1->con_route->roots.back())
	UPDATE_NOTE(r2->con_route->roots.front()) if (r2->con_route->roots.size() > 1) UPDATE_NOTE(r2->con_route->roots.back())
	continue;
}
if (r1->system->devel())
{	log << "Ignoring line matching highway in system in development: " << trim_line << '\n';
	splist << orig_line << endlines[l];
	continue;
}
// r1 and r2 are route matches, and we need to find
// waypoint indices, ignoring case and leading
// '+' or '*' when matching
while (*fields[2] == '*' || *fields[2] == '+') fields[2]++;
while (*fields[5] == '*' || *fields[5] == '+') fields[5]++;
upper(fields[2]);
upper(fields[5]);
// look for point indices for labels, first in pri_label_hash
std::unordered_map<std::string,unsigned int>::iterator lit1 = r1->pri_label_hash.find(fields[2]);
std::unordered_map<std::string,unsigned int>::iterator lit2 = r2->pri_label_hash.find(fields[5]);
// and then if not found, in alt_label_hash
if (lit1 == r1->pri_label_hash.end()) lit1 = r1->alt_label_hash.find(fields[2]);
if (lit2 == r2->pri_label_hash.end()) lit2 = r2->alt_label_hash.find(fields[5]);
// if we did not find matches for both labels...
if (lit1 == r1->alt_label_hash.end() || lit2 == r2->alt_label_hash.end())
{	bool invalid_char = 0;
	for (char& c : trim_line)
	  if (iscntrl(c))
	  {	c = '?';
		invalid_char = 1;
	  }
	for (char* c = fields[2]; *c; c++) if (iscntrl(*c)) *c = '?';
	for (char* c = fields[5]; *c; c++) if (iscntrl(*c)) *c = '?';
	if (lit1 == r1->alt_label_hash.end() && lit2 == r2->alt_label_hash.end())
		log << "Waypoint labels " << fields[2] << " and " << fields[5] << " not found in line: " << trim_line;
	else {	log << "Waypoint ";
		if (lit1 == r1->alt_label_hash.end())
			log << lookup1 << ' ' << fields[2];
		else	log << lookup2 << ' ' << fields[5];
		log << " not found in line: " << trim_line;
	     }
	if (invalid_char) log << " [contains invalid character(s)]";
	log << '\n';
	splist << orig_line << endlines[l];
	if (lit1 == r1->alt_label_hash.end() && lit1 != lit2)	UPDATE_NOTE(r1)
	if (lit2 == r2->alt_label_hash.end()) /*^diff rtes^*/	UPDATE_NOTE(r2)
	continue;
}
// are either of the labels used duplicates?
char duplicate = 0;
if (r1->duplicate_labels.count(fields[2]))
{	log << r1->region->code << ": duplicate label " << fields[2] << " in " << r1->root << ".\n";
	duplicate = 1;
}
if (r2->duplicate_labels.count(fields[5]))
{	log << r2->region->code << ": duplicate label " << fields[5] << " in " << r2->root << ".\n";
	duplicate = 1;
}
if (duplicate)
{	splist << orig_line << endlines[l];
	log << "  Please report this error in the Travel Mapping forum.\n"
	    << "  Unable to parse line: " << trim_line << '\n';
	r1->system->mark_routes_in_use(lookup1, lookup2);
	r1->mark_label_in_use(fields[2]);
	r2->mark_label_in_use(fields[5]);
	continue;
}
bool reverse = 0;
unsigned int index1 = lit1->second;
unsigned int index2 = lit2->second;
// if both region/route combos point to the same chopped route...
if (r1 == r2)
     {	// if both labels reference the same waypoint...
	if (index1 == index2)
	{	log << "Equivalent waypoint labels mark zero distance traveled in line: " << trim_line << '\n';
		splist << orig_line << endlines[l];
		UPDATE_NOTE(r1)
		continue;
	}
	if (index1 <= index2)
		r1->store_traveled_segments(this, log, update, index1, index2);
	else	r1->store_traveled_segments(this, log, update, index2, index1);
	r1->mark_labels_in_use(fields[2], fields[5]);
     }
else {	// user log warning for DISCONNECTED_ROUTE errors
	if (r1->con_route->disconnected)
	{	std::unordered_set<Region*> region_set;
		for (Route* r : r1->con_route->roots)
		  if (r->is_disconnected())
		    region_set.insert(r->region);
		std::list<Region*> region_list(region_set.begin(), region_set.end());
		region_list.sort(sort_regions_by_code);
		for (Region* r : region_list)
		  log << r->code << (r == region_list.back() ? ':' : '/');
		log << " DISCONNECTED_ROUTE error in " << r1->con_route->readable_name()
		    << ".\n  Please report this error in the Travel Mapping forum"
		    << ".\n  Travels may potentially be shown incorrectly for line: " << trim_line << '\n';
	}
	// Is .list entry forward or backward?
	if (r1->rootOrder > r2->rootOrder)
	     {	std::swap(r1, r2);
		index1 = lit2->second;
		index2 = lit1->second;
		reverse = 1;
		r1->mark_label_in_use(fields[5]);
		r2->mark_label_in_use(fields[2]);
	     }
	else {	r1->mark_label_in_use(fields[2]);
		r2->mark_label_in_use(fields[5]);
	     }
	// mark the beginning chopped route from index1 to its end
	if (r1->is_reversed())
		r1->store_traveled_segments(this, log, update, 0, index1);
	else	r1->store_traveled_segments(this, log, update, index1, r1->segment_list.size());
	// mark the ending chopped route from its beginning to index2
	if (r2->is_reversed())
		r2->store_traveled_segments(this, log, update, index2, r2->segment_list.size());
	else	r2->store_traveled_segments(this, log, update, 0, index2);
	// mark any intermediate chopped routes in their entirety.
	for (size_t r = r1->rootOrder+1; r < r2->rootOrder; r++)
	  r1->con_route->roots[r]->store_traveled_segments(this, log, update, 0, r1->con_route->roots[r]->segment_list.size());
     }
r1->system->mark_routes_in_use(lookup1, lookup2);
list_entries++;
// new .list lines for region split-ups
if (Args::splitregion == r1->region->code || Args::splitregion == r2->region->code)
{
	#include "splitregion.cpp"
}
else	splist << orig_line << endlines[l];
