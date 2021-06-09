#include "HighwaySystem.h"
#include "../ConnectedRoute/ConnectedRoute.h"
#include "../Datacheck/Datacheck.h"
#include "../ErrorList/ErrorList.h"
#include "../Route/Route.h"
#include "../Waypoint/Waypoint.h"
#include "../../functions/upper.h"

void HighwaySystem::route_integrity(ErrorList& el)
{	for (Route* r : route_list)
	{	// check for unconnected chopped routes
		if (!r->con_route)
			el.add_error(systemname + ".csv: root " + r->root + " not matched by any connected route root.");

		// create label hashes and check for duplicates
		for (unsigned int index = 0; index < r->point_list.size(); index++)
		{	// ignore case and leading '+' or '*'
			const char* lbegin = r->point_list[index]->label.data();
			while (*lbegin == '+' || *lbegin == '*') lbegin++;
			std::string upper_label(lbegin);
			upper(upper_label.data());
			// if primary label not duplicated, add to pri_label_hash
			if (r->alt_label_hash.find(upper_label) != r->alt_label_hash.end())
			{	Datacheck::add(r, r->point_list[index]->label, "", "", "DUPLICATE_LABEL", "");
				r->duplicate_labels.insert(upper_label);
			}
			else if (!r->pri_label_hash.insert(std::pair<std::string, unsigned int>(upper_label, index)).second)
			{	Datacheck::add(r, r->point_list[index]->label, "", "", "DUPLICATE_LABEL", "");
				r->duplicate_labels.insert(upper_label);
			}
			for (std::string& a : r->point_list[index]->alt_labels)
			{	// create canonical AltLabels
				while (a[0] == '+' || a[0] == '*') a = a.data()+1;
				upper(a.data());
				// populate unused set
				r->unused_alt_labels.insert(a);
				// create label->index hashes and check if AltLabels duplicated
				std::unordered_map<std::string, unsigned int>::iterator A = r->pri_label_hash.find(a);
				if (A != r->pri_label_hash.end())
				{	Datacheck::add(r, r->point_list[A->second]->label, "", "", "DUPLICATE_LABEL", "");
					r->duplicate_labels.insert(a);
				}
				else if (!r->alt_label_hash.insert(std::pair<std::string, unsigned int>(a, index)).second)
				{	Datacheck::add(r, a, "", "", "DUPLICATE_LABEL", "");
					r->duplicate_labels.insert(a);
				}
			}
		}
	}

	for (ConnectedRoute* cr : con_route_list)
	{	if (cr->roots.empty()) continue; // because there could be an invalid _con.csv entry
		// check cr->roots[0] by itself outside of loop
		cr->roots[0]->con_mismatch();
		for (size_t i = 1; i < cr->roots.size(); i++)
		{	// check for mismatched route endpoints within connected routes
			#define q cr->roots[i-1]
			#define r cr->roots[i]
			r->con_mismatch();
			if ( q->point_list.size() > 1 && r->point_list.size() > 1 && !r->con_beg()->same_coords(q->con_end()) )
			{	if	( q->con_beg()->same_coords(r->con_beg()) )
					q->is_reversed = 1;
				else if ( q->con_end()->same_coords(r->con_end()) )
					r->is_reversed = 1;
				else if ( q->con_beg()->same_coords(r->con_end()) )
				{	q->is_reversed = 1;
					r->is_reversed = 1;
				}
				else
				{	Datacheck::add(r, r->con_beg()->label, "", "",
						       "DISCONNECTED_ROUTE", q->con_end()->root_at_label());
					Datacheck::add(q, q->con_end()->label, "", "",
						       "DISCONNECTED_ROUTE", r->con_beg()->root_at_label());
				}
			}
			#undef q
			#undef r
		}
	}
}
