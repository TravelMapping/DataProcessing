HighwaySegment::HighwaySegment(Waypoint *w1, Waypoint *w2, Route *rte)
{	waypoint1 = w1;
	waypoint2 = w2;
	route = rte;
	concurrent = 0;
}

std::string HighwaySegment::str()
{	return route->readable_name() + " " + waypoint1->label + " " + waypoint2->label;
}

bool HighwaySegment::add_clinched_by(TravelerList *traveler)
{	clin_mtx.lock();
	bool result = clinched_by.insert(traveler).second;
	clin_mtx.unlock();
	return result;
}

std::string HighwaySegment::csv_line(unsigned int id)
{	/* return csv line to insert into a table */
	return "'" + std::to_string(id) + "','" + std::to_string(waypoint1->point_num) + "','" + std::to_string(waypoint2->point_num) + "','" + route->root + "'";
}

double HighwaySegment::length()
{	/* return segment length in miles */
	return waypoint1->distance_to(waypoint2);
}

std::string HighwaySegment::segment_name()
{	/* compute a segment name based on names of all
	concurrent routes, used for graph edge labels */
	std::string segment_name;
	if (!concurrent)
	{	if (route->system->active_or_preview())
		  segment_name = route->list_entry_name();
	} else
	  for (HighwaySegment *cs : *concurrent)
	    if (cs->route->system->active_or_preview())
	    {	if (segment_name != "") segment_name += ",";
		segment_name += cs->route->list_entry_name();
	    }
	return segment_name;
}

unsigned int HighwaySegment::index()
{	// segment number:
	// return this segment's index within its route's segment_list vector
	for (unsigned int i = 0; i < route->segment_list.size(); i++)
	  if (route->segment_list[i] == this) return i;
	return -1;	// error; this segment not found in vector
}

std::string HighwaySegment::clinchedby_code(std::list<TravelerList*> *traveler_lists)
{	// Return a hexadecimal string encoding which travelers have clinched this segment, for use in "traveled" graph files
	// Each character stores info for traveler #n thru traveler #n+3
	// The first character stores traveler 0 thru traveler 3,
	// The second character stores traveler 4 thru traveler 7, etc.
	// For each character, the low-order bit stores traveler n, and the high bit traveler n+3.
	std::string code;
	std::vector<unsigned char> clinch_vector(ceil(double(traveler_lists->size())/4)*4, 0);
	for (TravelerList* t : clinched_by)
		clinch_vector.at(t->traveler_num) = 1;
	for (unsigned short t = 0; t < traveler_lists->size(); t += 4)
	{	unsigned char nibble = 0;
		nibble |= clinch_vector.at(t);
		nibble |= clinch_vector.at(t+1)*2;
		nibble |= clinch_vector.at(t+2)*4;
		nibble |= clinch_vector.at(t+3)*8;
		if (nibble < 10) nibble += '0';
		else nibble += 55;
		code.push_back(nibble);
	}
	return code;
}

#include "compute_stats.cpp"
