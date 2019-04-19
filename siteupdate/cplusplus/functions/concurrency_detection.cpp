// concurrency detection -- will augment our structure with list of concurrent
// segments with each segment (that has a concurrency)
cout << et.et() << "Concurrent segment detection." << flush;
filename = args.logfilepath+"/concurrencies.log";
ofstream concurrencyfile(filename.data());
timestamp = time(0);
concurrencyfile << "Log file created at: " << ctime(&timestamp);
for (HighwaySystem *h : highway_systems)
{   cout << '.' << flush;
    for (Route &r : h->route_list)
	for (HighwaySegment *s : r.segment_list)
	    if (s->waypoint1->colocated && s->waypoint2->colocated)
	        for ( Waypoint *w1 : *(s->waypoint1->colocated) )
	            if (w1->route != &r)
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
					std::list<HighwaySegment*>::iterator it = s->concurrent->begin();	//FIXME
					while (it != s->concurrent->end() && *it != other) it++;		//see HighwaySegment.h
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
if (args.splitregion != "")
{	for (HighwaySystem *h : highway_systems)
	{	if (splitsystems.find(h->systemname) == splitsystems.end()) continue;
		ofstream fralog;
		if (args.splitregionpath != "") fralog.open((args.splitregionpath + "/logs/" + h->systemname + "-concurrencies.log").data());
		for (Route &r : h->route_list)
		{	if (r.region->code.substr(0, args.splitregion.size()) != args.splitregion) continue;
			for (HighwaySegment *s : r.segment_list)
			  if (!s->concurrent)
				fralog << s->str() << " has no concurrencies\n";
			  //TODO: check for concurrent same designated+bannered route in different region
			  else if (s->concurrent->size() % 2)
			  {	fralog << "Odd number of concurrencies:\n";
				for (HighwaySegment *cs : *(s->concurrent))
					fralog << '\t' << cs->str() << '\n';
			  }
			
		}
		fralog.close();
	}
}
