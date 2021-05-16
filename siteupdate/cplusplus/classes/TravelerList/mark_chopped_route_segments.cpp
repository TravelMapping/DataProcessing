// find the route that matches and when we do, match labels
std::string lookup = std::string(fields[0]) + ' ' + fields[1];
upper(lookup.data());
// look for region/route combo, first in pri_list_hash
std::unordered_map<std::string,Route*>::iterator rit = Route::pri_list_hash.find(lookup);
// and then if not found, in alt_list_hash
if (rit == Route::pri_list_hash.end())
{	rit = Route::alt_list_hash.find(lookup);
	if (rit == Route::alt_list_hash.end())
	{	bool invalid_char = 0;
		for (char& c : trim_line)
		  if (iscntrl(c))
		  {	c = '?';
			invalid_char = 1;
		  }
		log << "Unknown region/highway combo in line: " << trim_line;
		if (invalid_char) log << " [contains invalid character(s)]";
		log << '\n';
		splist << orig_line << endlines[l];
		continue;
	}
	else	log << "Note: deprecated route name " << fields[1]
		    << " -> canonical name " << rit->second->list_entry_name() << " in line: " << trim_line << '\n';
}
Route* r = rit->second;
if (r->system->devel())
{	log << "Ignoring line matching highway in system in development: " << trim_line << '\n';
	splist << orig_line << endlines[l];
	continue;
}
// r is a route match, and we need to find
// waypoint indices, ignoring case and leading
// '+' or '*' when matching
unsigned int index1, index2;
while (*fields[2] == '*' || *fields[2] == '+') fields[2]++;
while (*fields[3] == '*' || *fields[3] == '+') fields[3]++;
upper(fields[2]);
upper(fields[3]);
// look for point indices for labels, first in pri_label_hash
std::unordered_map<std::string,unsigned int>::iterator lit1 = r->pri_label_hash.find(fields[2]);
std::unordered_map<std::string,unsigned int>::iterator lit2 = r->pri_label_hash.find(fields[3]);
// and then if not found, in alt_label_hash
if (lit1 == r->pri_label_hash.end()) lit1 = r->alt_label_hash.find(fields[2]);
if (lit2 == r->pri_label_hash.end()) lit2 = r->alt_label_hash.find(fields[3]);
// if we did not find matches for both labels...
if (lit1 == r->alt_label_hash.end() || lit2 == r->alt_label_hash.end())
{	bool invalid_char = 0;
	for (char& c : trim_line)
	  if (iscntrl(c))
	  {	c = '?';
		invalid_char = 1;
	  }
	for (char* c = fields[2]; *c; c++) if (iscntrl(*c)) *c = '?';
	for (char* c = fields[3]; *c; c++) if (iscntrl(*c)) *c = '?';
	if (lit1 == lit2)
		log << "Waypoint labels " << fields[2] << " and " << fields[3] << " not found in line: " << trim_line;
	else {	log << "Waypoint label ";
		log << (lit1 == r->alt_label_hash.end() ? fields[2] : fields[3]);
		log << " not found in line: " << trim_line;
	     }
	if (invalid_char) log << " [contains invalid character(s)]";
	log << '\n';
	splist << orig_line << endlines[l];
	if (routes.insert(r).second && r->last_update && update && r->last_update[0] >= *update)
		log << "Route updated " << r->last_update[0] << ": " << r->readable_name() << '\n';
	continue;
}
// are either of the labels used duplicates?
char duplicate = 0;
if (r->duplicate_labels.find(fields[2]) != r->duplicate_labels.end())
{	log << r->region->code << ": duplicate label " << fields[2] << " in " << r->root
	    << ". Please report this error in the TravelMapping forum"
	    << ". Unable to parse line: " << trim_line << '\n';
	duplicate = 1;
}
if (r->duplicate_labels.find(fields[3]) != r->duplicate_labels.end())
{	log << r->region->code << ": duplicate label " << fields[3] << " in " << r->root
	    << ". Please report this error in the TravelMapping forum"
	    << ". Unable to parse line: " << trim_line << '\n';
	duplicate = 1;
}
if (duplicate)
{	splist << orig_line << endlines[l];
	if (routes.insert(r).second && r->last_update && update && r->last_update[0] >= *update)
		log << "Route updated " << r->last_update[0] << ": " << r->readable_name() << '\n';
	continue;
}
// if both labels reference the same waypoint...
if (lit1->second == lit2->second)
{	log << "Equivalent waypoint labels mark zero distance traveled in line: " << trim_line << '\n';
	splist << orig_line << endlines[l];
	if (routes.insert(r).second && r->last_update && update && r->last_update[0] >= *update)
		log << "Route updated " << r->last_update[0] << ": " << r->readable_name() << '\n';
}
// otherwise both labels are valid; mark in use & proceed
else {	r->system->lniu_mtx.lock();
	r->system->listnamesinuse.insert(lookup);
	r->system->lniu_mtx.unlock();
	r->system->uarn_mtx.lock();
	r->system->unusedaltroutenames.erase(lookup);
	r->system->uarn_mtx.unlock();
	r->liu_mtx.lock();
	r->labels_in_use.insert(fields[2]);
	r->labels_in_use.insert(fields[3]);
	r->liu_mtx.unlock();
	r->ual_mtx.lock();
	r->unused_alt_labels.erase(fields[2]);
	r->unused_alt_labels.erase(fields[3]);
	r->ual_mtx.unlock();

	list_entries++;
	bool reverse = 0;
	if (lit1->second <= lit2->second)
	     {	index1 = lit1->second;
		index2 = lit2->second;
	     }
	else {	index1 = lit2->second;
		index2 = lit1->second;
		reverse = 1;
	     }
	r->store_traveled_segments(this, log, index1, index2);
	// new .list lines for region split-ups
	if (Args::splitregion == r->region->code)
	{
		#define r1 r
		#define r2 r
		#include "splitregion.cpp"
		#undef r1
		#undef r2
	}
	else	splist << orig_line << endlines[l];
     }
