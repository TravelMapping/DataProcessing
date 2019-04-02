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
	for (TravelerList *t : clinched_by) if (t == traveler)
	{	clin_mtx.unlock();
		return 0;
	}
	clinched_by.push_front(traveler);
	clin_mtx.unlock();
	return 1;
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

/*std::string HighwaySegment::concurrent_travelers_sanity_check()
{	if (route->system->devel()) return "";
	if (concurrent)
	  for (HighwaySegment *conc : *concurrent)
	  {	if (clinched_by.size() != conc->clinched_by.size())
		{	if (conc->route->system->devel()) continue;
			return "[" + str() + "] clinched by " + std::to_string(clinched_by.size()) + " travelers; [" \
			     + conc->str() + "] clinched by " + std::to_string(conc->clinched_by.size()) + '\n';
		}
		else	for (TravelerList *t : clinched_by)
			{	std::list<TravelerList*>::iterator ct;
				for (ct = conc->clinched_by.begin(); ct != conc->clinched_by.end(); ct++)
				  if (*ct == t) break;
				if (ct == conc->clinched_by.end())
				  return t->traveler_name + " has clinched [" + str() + "], but not [" + conc->str() + "]\n";
			}
	  }
	return "";
}//*/

#include "compute_stats.cpp"
