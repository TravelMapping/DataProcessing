// concurrency detection -- will augment our structure with list of concurrent
// segments with each segment (that has a concurrency)
ofstream concurrencyfile(Args::logfilepath+"/concurrencies.log");
timestamp = time(0);
concurrencyfile << "Log file created at: " << ctime(&timestamp);
for (HighwaySystem& h : HighwaySystem::syslist)
{   cout << '.' << flush;
    for (Route& r : h.routes)
    {	Waypoint* p = r.points.data;
	for (HighwaySegment& s : r.segments)
	{   if (!s.concurrent && p->colocated)
		for (Waypoint *w1 : *p->colocated)
		    if (w1 != p)
			if (p[1].colocated)
			{   for (Waypoint *w2 : *p[1].colocated)
				if (w2 == w1+1)
				{   // we *almost* don't need to perform this route check, but it's possible that
				    // route 1 & route 2's waypoint arrays can be stored back-to-back in memory,
				    // making the last point of one route adjacent to the 1st of another.
				    if (w1->route == w2->route)
					s.add_concurrency(concurrencyfile, w1);
				}
				else if (w2 == w1-1 && w1->route == w2->route)
				    s.add_concurrency(concurrencyfile, w2);
			}
			// check for a route U-turning on itself with
			// nothing else colocated at the U-turn point
			else if (p+1 == w1-1 && w1->route == &r)
			    s.add_concurrency(concurrencyfile, p+1);
	    p++;
	}
    }
}
cout << "!\n";

// When splitting a region, perform a sanity check on concurrencies in its systems
if (Args::splitregionpath != "")
{	for (HighwaySystem& h : HighwaySystem::syslist)
	{	if (!splitsystems.count(h.systemname)) continue;
		ofstream fralog(Args::splitregionpath + "/logs/" + h.systemname + "-concurrencies.log");
		for (Route& r : h.routes)
		{	if (r.region->code.substr(0, Args::splitregion.size()) != Args::splitregion) continue;
			for (HighwaySegment& s : r.segments)
				if (!s.concurrent)
					fralog << s.str() << " has no concurrencies\n";
				else if (s.concurrent->size() % 2)
				     {	fralog << "Odd number of concurrencies:\n";
					for (HighwaySegment *cs : *(s.concurrent))
					  fralog << '\t' << cs->str() << '\n';
				     }
				else {	// check for concurrent segment with same name+banner in different region
					unsigned int matches = 0;
					for (HighwaySegment *cs : *(s.concurrent))
					  if (cs->route->name_no_abbrev() == s.route->name_no_abbrev() && cs->route->region != s.route->region)
					    matches++;
					if (matches != 1)
					  fralog << s.str() << " has " << matches << " concurrent segments with same name+banner in different region(s) (1 expected)\n";
				     }
		}
		fralog.close();
	}
}
