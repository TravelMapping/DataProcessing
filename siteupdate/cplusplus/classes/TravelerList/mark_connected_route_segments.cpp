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
	// log updates for routes beginning/ending r1's ConnectedRoute
	Route* cr = r1->con_route->roots.front();
	if (routes.insert(cr).second && cr->last_update && update && cr->last_update[0] >= *update)
		  log << "Route updated " << (*cr->last_update)[0] << ": " << cr->readable_name() << '\n';
	if (r1->con_route->roots.size() > 1)
	{	Route* cr = r1->con_route->roots.back();
		if (routes.insert(cr).second && cr->last_update && update && cr->last_update[0] >= *update)
		  log << "Route updated " << (*cr->last_update)[0] << ": " << cr->readable_name() << '\n';
	}
	// log updates for routes beginning/ending r2's ConnectedRoute
	cr = r2->con_route->roots.front();
	if (routes.insert(cr).second && cr->last_update && update && cr->last_update[0] >= *update)
		  log << "Route updated " << (*cr->last_update)[0] << ": " << cr->readable_name() << '\n';
	if (r2->con_route->roots.size() > 1)
	{	Route* cr = r2->con_route->roots.back();
		if (routes.insert(cr).second && cr->last_update && update && cr->last_update[0] >= *update)
		  log << "Route updated " << (*cr->last_update)[0] << ": " << cr->readable_name() << '\n';
	}
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
	continue;
}
// are either of the labels used duplicates?
char duplicate = 0;
if (r1->duplicate_labels.find(fields[2]) != r1->duplicate_labels.end())
{	log << r1->region->code << ": duplicate label " << fields[2] << " in " << r1->root
	    << ". Please report this error in the TravelMapping forum"
	    << ". Unable to parse line: " << trim_line << '\n';
	duplicate = 1;
}
if (r2->duplicate_labels.find(fields[5]) != r2->duplicate_labels.end())
{	log << r2->region->code << ": duplicate label " << fields[5] << " in " << r2->root
	    << ". Please report this error in the TravelMapping forum"
	    << ". Unable to parse line: " << trim_line << '\n';
	duplicate = 1;
}
if (duplicate)
{	splist << orig_line << endlines[l];
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
		if (routes.insert(r1).second && r1->last_update && update && r1->last_update[0] >= *update)
			log << "Route updated " << (*r1->last_update)[0] << ": " << r1->readable_name() << '\n';
		continue;
	}
	if (index1 <= index2)
		r1->store_traveled_segments(this, log, index1, index2);
	else	r1->store_traveled_segments(this, log, index2, index1);
     }
else {	if (r1->rootOrder > r2->rootOrder)
	{	std::swap(r1, r2);
		index1 = lit2->second;
		index2 = lit1->second;
		reverse = 1;
	}
	// mark the beginning chopped route from index1 to its end
	if (r1->is_reversed)
		r1->store_traveled_segments(this, log, 0, index1);
	else	r1->store_traveled_segments(this, log, index1, r1->segment_list.size());
	// mark the ending chopped route from its beginning to index2
	if (r2->is_reversed)
		r2->store_traveled_segments(this, log, index2, r2->segment_list.size());
	else	r2->store_traveled_segments(this, log, 0, index2);
	// mark any intermediate chopped routes in their entirety.
	for (size_t r = r1->rootOrder+1; r < r2->rootOrder; r++)
	  r1->con_route->roots[r]->store_traveled_segments(this, log, 0, r1->con_route->roots[r]->segment_list.size());
     }
// both labels are valid; mark in use & proceed
r1->system->lniu_mtx.lock();
r1->system->listnamesinuse.insert(lookup1);
r1->system->lniu_mtx.unlock();
r1->system->uarn_mtx.lock();
r1->system->unusedaltroutenames.erase(lookup1);
r1->system->uarn_mtx.unlock();
r1->liu_mtx.lock();
r1->labels_in_use.insert(fields[2]);
r1->liu_mtx.unlock();
r1->ual_mtx.lock();
r1->unused_alt_labels.erase(fields[2]);
r1->ual_mtx.unlock();
r2->system->lniu_mtx.lock();
r2->system->listnamesinuse.insert(lookup2);
r2->system->lniu_mtx.unlock();
r2->system->uarn_mtx.lock();
r2->system->unusedaltroutenames.erase(lookup2);
r2->system->uarn_mtx.unlock();
r2->liu_mtx.lock();
r2->labels_in_use.insert(fields[5]);
r2->liu_mtx.unlock();
r2->ual_mtx.lock();
r2->unused_alt_labels.erase(fields[5]);
r2->ual_mtx.unlock();
list_entries++;
// new .list lines for region split-ups
if (Args::splitregion == r1->region->code || Args::splitregion == r2->region->code)
{
	#include "splitregion.cpp"
}
else	splist << orig_line << endlines[l];
