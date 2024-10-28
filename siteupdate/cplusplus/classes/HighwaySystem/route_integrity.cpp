#include "HighwaySystem.h"
#include "../ConnectedRoute/ConnectedRoute.h"
#include "../Datacheck/Datacheck.h"
#include "../ErrorList/ErrorList.h"
#include "../Route/Route.h"
#include "../Waypoint/Waypoint.h"

void HighwaySystem::route_integrity(ErrorList& el)
{	for (Route& r : routes)
	{	// check for unconnected chopped routes
		if (!r.con_route)
			el.add_error(systemname + ".csv: root " + r.root + " not matched by any connected route root.");
		// per-route CSV datachecks
		else	r.con_mismatch();
		#define CSV_LINE r.system->systemname + ".csv#L" + std::to_string(r.index()+2)
		if (r.abbrev.empty())
		{	if ( r.banner.size() && !strncmp(r.banner.data(), r.city.data(), r.banner.size()) )
			  Datacheck::add(&r, "", "", "", "ABBREV_AS_CHOP_BANNER", CSV_LINE);
		} else	if (r.city.empty())
			  Datacheck::add(&r, "", "", "", "ABBREV_NO_CITY", CSV_LINE);
		#undef CSV_LINE

		// per-waypoint colocation-based datachecks
		for (Waypoint& w : r.points)
		{	if (!w.is_hidden)
			{	w.label_selfref();
				// "visible front" flavored VISIBLE_HIDDEN_COLOC check
				if (w.colocated && &w == w.colocated->front())
				  for (auto p = ++w.colocated->begin(), end = w.colocated->end(); p != end; p++)
				    if ((*p)->is_hidden)
				    {	Datacheck::add(w.route, w.label, "", "", "VISIBLE_HIDDEN_COLOC", (*p)->root_at_label());
					break;
				    }
			}	// "hidden front" flavored VHC is handled via Waypoint::hidden_junction below
			else	w.hidden_junction();
			//#include "unexpected_designation.cpp"
		}

		r.create_label_hashes();
	}

	for (ConnectedRoute& cr : con_routes)
	{	cr.verify_connectivity();
		cr.combine_con_routes();
	}
}
