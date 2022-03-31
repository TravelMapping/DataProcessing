#include "TravelerList.h"
#include "../Args/Args.h"
#include "../ConnectedRoute/ConnectedRoute.h"
#include "../HighwaySystem/HighwaySystem.h"
#include "../Region/Region.h"
#include "../Route/Route.h"
#include "../../functions/format_clinched_mi.h"
#include <cstring>
#include <fstream>

void TravelerList::userlog(const double total_active_only_miles, const double total_active_preview_miles)
{	char fstr[112];
	std::cout << "." << std::flush;
	std::ofstream log(Args::logfilepath+"/users/"+traveler_name+".log", std::ios::app);
	log << "Clinched Highway Statistics\n";
	log << "Overall in active systems: " << format_clinched_mi(active_only_miles(), total_active_only_miles) << '\n';
	log << "Overall in active+preview systems: " << format_clinched_mi(active_preview_miles(), total_active_preview_miles) << '\n';

	log << "Overall by region: (each line reports active only then active+preview)\n";
	std::list<Region*> travregions;
	for (std::pair<Region* const, double> &rm : active_preview_mileage_by_region)
		travregions.push_back(rm.first);
	travregions.sort(sort_regions_by_code);
	for (Region *region : travregions)
	{	double t_active_miles = 0;
		if (active_only_mileage_by_region.count(region))
			t_active_miles = active_only_mileage_by_region.at(region);
		log << region->code << ": " << format_clinched_mi(t_active_miles, region->active_only_mileage) << ", "
		    << format_clinched_mi(active_preview_mileage_by_region.at(region), region->active_preview_mileage) << '\n';
	}

	unsigned int active_systems = 0;
	unsigned int preview_systems = 0;

	// present stats by system here, also generate entries for
	// DB table clinchedSystemMileageByRegion as we compute and
	// have the data handy
	for (HighwaySystem *h : HighwaySystem::syslist)
	  if (h->active_or_preview())
	  {	if (h->active()) active_systems++;
		else	preview_systems++;
		double t_system_overall = 0;
		if (system_region_mileages.count(h))
			t_system_overall = system_region_miles(h);
		if (t_system_overall)
		{	if (h->active())
				active_systems_traveled++;
			else	preview_systems_traveled++;
			if (float(t_system_overall) == float(h->total_mileage()))
			  if (h->active())
				active_systems_clinched++;
			  else	preview_systems_clinched++;

			// stats by region covered by system, always in csmbr for
			// the DB, but add to logs only if it's been traveled at
			// all and it covers multiple regions
			auto& systemname = h->systemname;
			auto& sysmbr = h->mileage_by_region;
			log << "System " << systemname << " (" << h->level_name() << ") overall: "
			    << format_clinched_mi(t_system_overall, h->total_mileage()) << '\n';
			if (sysmbr.size() > 1)
				log << "System " << systemname << " by region:\n";
			std::list<Region*> sysregions;
			for (std::pair<Region* const, double> &rm : sysmbr)
				sysregions.push_back(rm.first);
			sysregions.sort(sort_regions_by_code);
			for (Region *region : sysregions)
			{	double system_region_mileage = 0;
				auto it = system_region_mileages.at(h).find(region);
				if (it != system_region_mileages.at(h).end())
					system_region_mileage = it->second;
				if (sysmbr.size() > 1)
					log << "  " << region->code << ": " << format_clinched_mi(system_region_mileage, sysmbr.at(region)) << '\n';
			}

			// stats by highway for the system, by connected route and
			// by each segment crossing region boundaries if applicable
			unsigned int num_con_rtes_traveled = 0;
			unsigned int num_con_rtes_clinched = 0;
			log << "System " << systemname << " by route (traveled routes only):\n";
			for (ConnectedRoute *cr : h->con_route_list)
			{	double con_clinched_miles = 0;
				std::string to_write = "";
				auto& roots = cr->roots;
				for (Route *r : roots)
				{	// find traveled mileage on this by this user
					double miles = r->clinched_by_traveler(this);
					if (miles)
					{	cr_values.emplace_back(r, miles);
						con_clinched_miles += miles;
						to_write += "  " + r->readable_name() + ": " + format_clinched_mi(miles,r->mileage) + "\n";
					}
				}
				if (con_clinched_miles)
				{	num_con_rtes_traveled += 1;
					num_con_rtes_clinched += (con_clinched_miles == cr->mileage);
					ccr_values.emplace_back(cr, con_clinched_miles);
					log << cr->readable_name() << ": " << format_clinched_mi(con_clinched_miles, cr->mileage) << '\n';
					if (roots.size() == 1)
						log << " (" << roots[0]->readable_name() << " only)\n";
					else	log << to_write << '\n';
				}
			}
			sprintf(fstr, " connected routes traveled: %i of %i (%.1f%%), clinched: %i of %i (%.1f%%).",
				num_con_rtes_traveled, (int)h->con_route_list.size(), 100*(double)num_con_rtes_traveled/h->con_route_list.size(),
				num_con_rtes_clinched, (int)h->con_route_list.size(), 100*(double)num_con_rtes_clinched/h->con_route_list.size());
			log << "System " << systemname << fstr << '\n';
		}
	  }

	// grand summary, active only
	sprintf(fstr,"\nTraveled %i of %i (%.1f%%), Clinched %i of %i (%.1f%%) active systems",
			active_systems_traveled, active_systems, 100*(double)active_systems_traveled/active_systems,
			active_systems_clinched, active_systems, 100*(double)active_systems_clinched/active_systems);
	log << fstr << '\n';
	// grand summary, active+preview
	sprintf(fstr,"Traveled %i of %i (%.1f%%), Clinched %i of %i (%.1f%%) preview systems",
			preview_systems_traveled, preview_systems, 100*(double)preview_systems_traveled/preview_systems,
			preview_systems_clinched, preview_systems, 100*(double)preview_systems_clinched/preview_systems);
	log << fstr << '\n';

	// updated routes, sorted by date
	log << "\nMost recent updates for listed routes:\n";
	std::list<Route*> route_list;
	for (Route* r : updated_routes) if (r->last_update) route_list.push_back(r);
	updated_routes.clear();
	route_list.sort(sort_route_updates_oldest);
	for (Route* r : route_list)
	    log	<< r->last_update[0] << " | " << r->last_update[1] << " | " << r->last_update[2] << " | "
		<< r->last_update[3] << " | " << r->last_update[4] << '\n';

	log.close();
}
