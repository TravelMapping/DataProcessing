// concurrency detection -- will augment our structure with list of concurrent
// segments with each segment (that has a concurrency)
cout << et.et() << "Concurrent segment detection." << flush;
ofstream concurrencyfile(Args::logfilepath+"/concurrencies.log");
timestamp = time(0);
concurrencyfile << "Log file created at: " << ctime(&timestamp);
for (HighwaySystem *h : HighwaySystem::syslist)
{   cout << '.' << flush;
    for (Route *r : h->route_list)
	for (HighwaySegment *s : r->segment_list)
	    if (s->waypoint1->colocated && s->waypoint2->colocated)
	        for ( Waypoint *w1 : *(s->waypoint1->colocated) )
	            if (w1->route != r)
	                for ( Waypoint *w2 : *(s->waypoint2->colocated) )
	                    if (w1->route == w2->route)
			    {   HighwaySegment *other = w1->route->find_segment_by_waypoints(w1,w2);
	                        if (other)
	                            if (!s->concurrent)
	                            {   s->concurrent = new list<HighwaySegment*>;
							// deleted on termination of program
	                                other->concurrent = s->concurrent;
	                                s->concurrent->push_back(s);
	                                s->concurrent->push_back(other);
	                                concurrencyfile << "New concurrency [" << s->str() << "][" << other->str() << "] (" << s->concurrent->size() << ")\n";
				    }
	                            else
	                            {   other->concurrent = s->concurrent;
					std::list<HighwaySegment*>::iterator it = s->concurrent->begin();
					while (it != s->concurrent->end() && *it != other) it++;
	                                if (it == s->concurrent->end())
	                                {   s->concurrent->push_back(other);
	                                    //concurrencyfile << "Added concurrency [" << s->str() << "]-[" \
							      << other->str() << "] (" << s->concurrent->size() << ")\n";
	                                    concurrencyfile << "Extended concurrency ";
	                                    for (HighwaySegment *x : *(s->concurrent))
	                                        concurrencyfile << '[' << x->str() << ']';
	                                    concurrencyfile << " (" << s->concurrent->size() << ")\n";
					}
				    }
			    }
    // see https://github.com/TravelMapping/DataProcessing/issues/137
    // changes not yet implemented in either the original Python or this C++ version.
}
cout << "!\n";

// When splitting a region, perform a sanity check on concurrencies in its systems
if (Args::splitregion != "")
{	for (HighwaySystem *h : HighwaySystem::syslist)
	{	if (splitsystems.find(h->systemname) == splitsystems.end()) continue;
		ofstream fralog;
		if (Args::splitregionpath != "") fralog.open(Args::splitregionpath + "/logs/" + h->systemname + "-concurrencies.log");
		for (Route *r : h->route_list)
		{	if (r->region->code.substr(0, Args::splitregion.size()) != Args::splitregion) continue;
			for (HighwaySegment *s : r->segment_list)
				if (!s->concurrent)
					fralog << s->str() << " has no concurrencies\n";
				else if (s->concurrent->size() % 2)
				     {	fralog << "Odd number of concurrencies:\n";
					for (HighwaySegment *cs : *(s->concurrent))
					  fralog << '\t' << cs->str() << '\n';
				     }
				else {	// check for concurrent segment with same name+banner in different region
					unsigned int matches = 0;
					for (HighwaySegment *cs : *(s->concurrent))
					  if (cs->route->name_no_abbrev() == s->route->name_no_abbrev() && cs->route->region != s->route->region)
					    matches++;
					if (matches != 1)
					  fralog << s->str() << " has " << matches << " concurrent segments with same name+banner in different region(s) (1 expected)" << '\n';
				     }
		}
		fralog.close();
	}
}
