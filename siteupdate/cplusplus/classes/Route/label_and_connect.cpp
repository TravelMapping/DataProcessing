#include "Route.h"
#include "../ConnectedRoute/ConnectedRoute.h"
#include "../DatacheckEntry/DatacheckEntry.h"
#include "../ErrorList/ErrorList.h"
#include "../HighwaySystem/HighwaySystem.h"
#include "../Waypoint/Waypoint.h"
#include "../../functions/upper.h"

void Route::label_and_connect(ErrorList& el)
{	// check for unconnected chopped routes
	if (!con_route)
	{	el.add_error(system->systemname + ".csv: root " + root + " not matched by any connected route root.");
		return;
	}

	// check for mismatched route endpoints within connected routes
	#define q con_route->roots[rootOrder-1]
	if ( rootOrder > 0 && q->point_list.size() > 1 && point_list.size() > 1 && !con_beg()->same_coords(q->con_end()) )
	{	if ( q->con_beg()->same_coords(con_beg()) )
		{	//std::cout << "DEBUG: marking only " << q->str() << " reversed" << std::endl;
			//if (q->is_reversed) std::cout << "DEBUG: " << q->str() << " already reversed!" << std::endl;
			q->is_reversed = 1;
		}
		else if ( q->con_end()->same_coords(con_end()) )
		{	//std::cout << "DEBUG: marking only " <<    str() << " reversed" << std::endl;
			//if (   is_reversed) std::cout << "DEBUG: " <<    str() << " already reversed!" << std::endl;
			is_reversed = 1;
		}
		else if ( q->con_beg()->same_coords(con_end()) )
		{	//std::cout << "DEBUG: marking both " << q->str() << " and " << str() << " reversed" << std::endl;
			//if (q->is_reversed) std::cout << "DEBUG: " << q->str() << " already reversed!" << std::endl;
			//if (   is_reversed) std::cout << "DEBUG: " <<    str() << " already reversed!" << std::endl;
			q->is_reversed = 1;
			is_reversed = 1;
		}
		else
		{	DatacheckEntry::add(this, con_beg()->label, "", "",
					    "DISCONNECTED_ROUTE", q->con_end()->root_at_label());
			DatacheckEntry::add(q, q->con_end()->label, "", "",
					    "DISCONNECTED_ROUTE", con_beg()->root_at_label());
		}
	}
	#undef q

	// create label hashes and check for duplicates
	for (unsigned int index = 0; index < point_list.size(); index++)
	{	// ignore case and leading '+' or '*'
		const char* lbegin = point_list[index]->label.data();
		while (*lbegin == '+' || *lbegin == '*') lbegin++;
		std::string upper_label(lbegin);
		upper(upper_label.data());
		// if primary label not duplicated, add to pri_label_hash
		if (alt_label_hash.find(upper_label) != alt_label_hash.end())
		{	DatacheckEntry::add(this, upper_label, "", "", "DUPLICATE_LABEL", "");
			duplicate_labels.insert(upper_label);
		}
		else if (!pri_label_hash.insert(std::pair<std::string, unsigned int>(upper_label, index)).second)
		{	DatacheckEntry::add(this, upper_label, "", "", "DUPLICATE_LABEL", "");
			duplicate_labels.insert(upper_label);
		}
		for (std::string& a : point_list[index]->alt_labels)
		{	// create canonical AltLabels
			while (a[0] == '+' || a[0] == '*') a = a.data()+1;
			upper(a.data());
			// populate unused set
			unused_alt_labels.insert(a);
			// create label->index hashes and check if AltLabels duplicated
			if (pri_label_hash.find(a) != pri_label_hash.end())
			{	DatacheckEntry::add(this, a, "", "", "DUPLICATE_LABEL", "");
				duplicate_labels.insert(a);
			}
			else if (!alt_label_hash.insert(std::pair<std::string, unsigned int>(a, index)).second)
			{	DatacheckEntry::add(this, a, "", "", "DUPLICATE_LABEL", "");
				duplicate_labels.insert(a);
			}
		}
	}
}
