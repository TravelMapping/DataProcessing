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
	  for (size_t i = 1; i < cr.roots.size(); i++)
	  {	// check for mismatched route endpoints within connected routes
		auto& q = cr.roots[i-1];
		auto& r = cr.roots[i];
		auto flag = [&]()
		{	Datacheck::add(q, q->con_end()->label, "", "", "DISCONNECTED_ROUTE",  r->points[0].root_at_label());
			Datacheck::add(r,  r->points[0].label, "", "", "DISCONNECTED_ROUTE", q->con_end()->root_at_label());
			cr.disconnected = 1;
			q->set_disconnected();
			r->set_disconnected();
		};
		if ( q->points.size > 1 && r->points.size > 1 && !r->points.begin()->same_coords(q->con_end()) )
			if	( q->con_end()->same_coords(&r->points.back()) )	// R can be reversed
			  if	( q->con_beg()->same_coords(r->points.begin())		// Can Q be reversed instead?
			  &&	( q == cr.roots[0] || q->is_disconnected() )		// Is Q not locked into one direction?
			  &&	( i+1 < cr.roots.size() )				// Is there another chopped route after R?
			  &&	(    r->points.back().same_coords(cr.roots[i+1]->points.begin())	// And does its beginning
				  || r->points.back().same_coords(&cr.roots[i+1]->points.back()) ))	// or end link to R as-is?
				q->set_reversed();
			  else	r->set_reversed();
			else if ( q->con_beg()->same_coords(&r->points.back()) )	// Q & R can both be reversed together
			  if	( q == cr.roots[0] || q->is_disconnected() )		// as long as Q's direction is unknown
			  {	q->set_reversed();
				r->set_reversed();
			  }
			  else	flag();
			else if ( q->con_beg()->same_coords(r->points.begin())		// Only Q can be reversed
			     && ( q == cr.roots[0] || q->is_disconnected() ))		// as long as its direction is unknown
				q->set_reversed();
			else	flag();
	  }
}
