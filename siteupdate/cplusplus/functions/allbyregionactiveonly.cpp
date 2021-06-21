#include "../classes/Args/Args.h"
#include "../classes/Region/Region.h"
#include "../classes/TravelerList/TravelerList.h"
#include "../threads/threads.h"
#include <fstream>

void allbyregionactiveonly(std::mutex* mtx)
{	double total_mi;
	char fstr[112];
	std::ofstream allfile(Args::csvstatfilepath + "/allbyregionactiveonly.csv");
	allfile << "Traveler,Total";
	std::list<Region*> regions;
	total_mi = 0;
	for (Region* r : Region::allregions)
	  if (r->active_only_mileage)
	  {	regions.push_back(r);
		total_mi += r->active_only_mileage;
	  }
	regions.sort(sort_regions_by_code);
	for (Region *region : regions)
		allfile << ',' << region->code;
	allfile << '\n';
	for (TravelerList *t : TravelerList::allusers)
	{	double t_total_mi = 0;
		for (std::pair<Region* const, double>& rm : t->active_only_mileage_by_region)
			t_total_mi += rm.second;
		sprintf(fstr, "%.2f", t_total_mi);
		allfile << t->traveler_name << ',' << fstr;
		for (Region *region : regions)
		  try {	sprintf(fstr, "%.2f", t->active_only_mileage_by_region.at(region));
			allfile << ',' << fstr;
		      }
		  catch (const std::out_of_range& oor)
		      {	allfile << ",0";
		      }
		allfile << '\n';
	}
	sprintf(fstr, "TOTAL,%.2f", total_mi);
	allfile << fstr;
	for (Region *region : regions)
	{	sprintf(fstr, ",%.2f", region->active_only_mileage);
		allfile << fstr;
	}
	allfile << '\n';
	allfile.close();

#ifdef threading_enabled
	if (mtx)
	StatsCsvThread(0, mtx);
#endif
}
