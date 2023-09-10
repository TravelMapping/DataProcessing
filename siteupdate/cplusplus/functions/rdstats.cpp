#include "../classes/Args/Args.h"
#include "../classes/ConnectedRoute/ConnectedRoute.h"
#include "../classes/HighwaySystem/HighwaySystem.h"
#include "../classes/Region/Region.h"
#include "../classes/Route/Route.h"
#include <fstream>
#include <list>

void rdstats(double& active_only_miles, double& active_preview_miles, time_t* timestamp)
{	char fstr[112];
	std::ofstream rdstatsfile(Args::logfilepath+"/routedatastats.log");
	*timestamp = time(0);
	rdstatsfile << "Travel Mapping highway mileage as of " << ctime(timestamp);

	double overall_miles = 0;
	for (Region& r : Region::allregions)
	{	active_only_miles += r.active_only_mileage;
		active_preview_miles += r.active_preview_mileage;
		overall_miles += r.overall_mileage;
	}
	sprintf(fstr, "Active routes (active): %.2f mi\n", active_only_miles);
	rdstatsfile << fstr;
	sprintf(fstr, "Clinchable routes (active, preview): %.2f mi\n", active_preview_miles);
	rdstatsfile << fstr;
	sprintf(fstr, "All routes (active, preview, devel): %.2f mi\n", overall_miles);
	rdstatsfile << fstr;
	rdstatsfile << "Breakdown by region:\n";
	// a nice enhancement later here might break down by continent, then country,
	// then region
	for (Region& region : Region::allregions)
	  if (region.overall_mileage)
	  {	sprintf(fstr, ": %.2f (active), %.2f (active, preview) %.2f (active, preview, devel)\n",
			region.active_only_mileage, region.active_preview_mileage, region.overall_mileage);
		rdstatsfile << region.code << fstr;
	  }

	for (HighwaySystem& h : HighwaySystem::syslist)
	{	sprintf(fstr, ") total: %.2f mi\n", h.total_mileage());
		rdstatsfile << "System " << h.systemname << " (" << h.level_name() << fstr;
		if (h.mileage_by_region.size() > 1)
		{	rdstatsfile << "System " << h.systemname << " by region:\n";
			std::list<Region*> regions_in_system;
			for (auto& rm : h.mileage_by_region)
				regions_in_system.push_back(rm.first);
			regions_in_system.sort();
			for (Region *r : regions_in_system)
			{	sprintf(fstr, ": %.2f mi\n", h.mileage_by_region.at(r));
				rdstatsfile << r->code << fstr;
			}
		}
		rdstatsfile << "System " << h.systemname << " by route:\n";
		for (ConnectedRoute& cr : h.con_routes)
		{	std::string to_write = "";
			for (Route *r : cr.roots)
			{	sprintf(fstr, ": %.2f mi\n", r->mileage);
				to_write += "  " + r->readable_name() + fstr;
				cr.mileage += r->mileage;
			}
			sprintf(fstr, ": %.2f mi", cr.mileage);
			rdstatsfile << cr.readable_name() << fstr;
			if (cr.roots.size() == 1)
				rdstatsfile << " (" << cr.roots[0]->readable_name() << " only)\n";
			else	rdstatsfile << '\n' << to_write;
		}
	}
	rdstatsfile.close();
}
