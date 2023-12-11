#include "HighwaySystem.h"
#include "../ConnectedRoute/ConnectedRoute.h"
#include "../Datacheck/Datacheck.h"
#include "../ErrorList/ErrorList.h"
#include "../Route/Route.h"
#include "../Waypoint/Waypoint.h"
#include "../../functions/tmstring.h"
#include <cstring>

void HighwaySystem::route_integrity(ErrorList& el)
{	for (Route& r : routes)
	{	// check for unconnected chopped routes
		if (!r.con_route)
			el.add_error(systemname + ".csv: root " + r.root + " not matched by any connected route root.");

		// per-route CSV datachecks
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
			}
			else	w.hidden_junction();
			//#include "unexpected_designation.cpp"
		}

		// Yes, these two loops could be combined into one.
		// But that performs worse, probably due to increased cache evictions.

		// create label hashes and check for duplicates
		for (unsigned int index = 0; index < r.points.size; index++)
		{	// ignore case and leading '+' or '*'
			const char* lbegin = r.points[index].label.data();
			while (*lbegin == '+' || *lbegin == '*') lbegin++;
			std::string upper_label(lbegin);
			upper(upper_label.data());
			// if primary label not duplicated, add to pri_label_hash
			if (r.alt_label_hash.count(upper_label))
			{	Datacheck::add(&r, r.points[index].label, "", "", "DUPLICATE_LABEL", "");
				r.duplicate_labels.insert(upper_label);
			}
			else if (!r.pri_label_hash.insert(std::pair<std::string, unsigned int>(upper_label, index)).second)
			{	Datacheck::add(&r, r.points[index].label, "", "", "DUPLICATE_LABEL", "");
				r.duplicate_labels.insert(upper_label);
			}
			for (std::string& a : r.points[index].alt_labels)
			{	// create canonical AltLabels
				while (a[0] == '+' || a[0] == '*') a = a.data()+1;
				upper(a.data());
				// populate unused set
				r.unused_alt_labels.insert(a);
				// create label->index hashes and check if AltLabels duplicated
				std::unordered_map<std::string, unsigned int>::iterator A = r.pri_label_hash.find(a);
				if (A != r.pri_label_hash.end())
				{	Datacheck::add(&r, r.points[A->second].label, "", "", "DUPLICATE_LABEL", "");
					r.duplicate_labels.insert(a);
				}
				else if (!r.alt_label_hash.insert(std::pair<std::string, unsigned int>(a, index)).second)
				{	Datacheck::add(&r, a, "", "", "DUPLICATE_LABEL", "");
					r.duplicate_labels.insert(a);
				}
			}
		}
	}

	for (ConnectedRoute& cr : con_routes)
	{	if (cr.roots.empty()) continue; // because there could be an invalid _con.csv entry
		// check cr.roots[0] by itself outside of loop
		cr.roots[0]->con_mismatch();
		for (size_t i = 1; i < cr.roots.size(); i++)
		{	// check for mismatched route endpoints within connected routes
			auto& q = cr.roots[i-1];
			auto& r = cr.roots[i];
			r->con_mismatch();
			if ( q->points.size > 1 && r->points.size > 1 && !r->con_beg()->same_coords(q->con_end()) )
			{	if	( q->con_beg()->same_coords(r->con_beg()) )
					q->set_reversed();
				else if ( q->con_end()->same_coords(r->con_end()) )
					r->set_reversed();
				else if ( q->con_beg()->same_coords(r->con_end()) )
				{	q->set_reversed();
					r->set_reversed();
				}
				else
				{	Datacheck::add(r, r->con_beg()->label, "", "",
						       "DISCONNECTED_ROUTE", q->con_end()->root_at_label());
					Datacheck::add(q, q->con_end()->label, "", "",
						       "DISCONNECTED_ROUTE", r->con_beg()->root_at_label());
					cr.disconnected = 1;
					q->set_disconnected();
					r->set_disconnected();
				}
			}
		}
	}
}
