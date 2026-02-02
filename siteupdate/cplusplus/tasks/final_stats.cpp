cout << et.et() << "Processed " << HighwaySystem::syslist.size << " highway systems." << endl;
unsigned int routes = 0;
unsigned int points = 0;
unsigned int segments = 0;
for (HighwaySystem& h : HighwaySystem::syslist)
{	routes += h.routes.size;
	for (Route& r : h.routes)
	{	points += r.points.size;
		segments += r.segments.size;
	}
}
cout << "Processed " << routes << " routes with a total of " << points << " points and " << segments << " segments." << endl;
if (points != all_waypoints.size())
  cout << "MISMATCH: all_waypoints contains " << all_waypoints.size() << " waypoints!" << endl;
cout << et.et() << "WaypointQuadtree contains " << all_waypoints.total_nodes() << " total nodes." << endl;

if (!Args::errorcheck)
{	// compute colocation of waypoints stats
        cout << et.et() << "Computing waypoint colocation stats";
        if (Args::colocationlimit)
		cout << ", reporting all with " << Args::colocationlimit << " or more colocations:" << endl;
	else	cout << "." << endl;
	all_waypoints.final_report(colocate_counts);
	cout << "Waypoint colocation counts:" << endl;
	unsigned int unique_locations = 0;
	for (unsigned int c = 1; c < colocate_counts.size(); c++)
	{	unique_locations += colocate_counts[c];
		printf("%6i are each occupied by %2i waypoints.\n", colocate_counts[c], c);
	}
	cout << "Unique locations: " << unique_locations << endl;
} else	all_waypoints.final_report(colocate_counts);
