/* duplicate the old canonical_waypoint_name-based functionality
if (w.colocated && w.route->system->active_or_preview())
{	std::vector<Waypoint*> ap_coloc;
	for (Waypoint* p : *w.colocated) if (p->route->system->active_or_preview()) ap_coloc.push_back(p);
	if (ap_coloc.size() == 2)
	{	Waypoint* other = &w == ap_coloc[1] ? ap_coloc[0] : ap_coloc[1];
		w.label_references_route(other->route);
	}
}//*/

if (w.colocated && w.colocated->size() == 2)
{	Waypoint* other = &w == w.colocated->back() ? w.colocated->front() : w.colocated->back();
	w.label_references_route(other->route);
}
