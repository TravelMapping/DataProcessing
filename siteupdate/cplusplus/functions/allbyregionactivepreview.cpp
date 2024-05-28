#define FMT_HEADER_ONLY
#include "../classes/Args/Args.h"
#include "../classes/Region/Region.h"
#include "../classes/TravelerList/TravelerList.h"
#include "../threads/threads.h"
#include <fmt/format.h>
#include <fstream>

void allbyregionactivepreview(std::mutex* mtx, double total_mi)
{	char fstr[112];
	std::ofstream allfile(Args::csvstatfilepath + "/allbyregionactivepreview.csv");
	allfile << "Traveler,Total";
	std::list<Region*> regions;
	for (Region& r : Region::allregions)
	  if (r.active_preview_mileage)
	  {	regions.push_back(&r);
		allfile << ',' << r.code;
	  }
	allfile << '\n';
	for (TravelerList& t : TravelerList::allusers)
	{	double t_total_mi = 0;
		for (std::pair<Region* const, double>& rm : t.active_preview_mileage_by_region)
			t_total_mi += rm.second;
		*fmt::format_to(fstr, "{:.2f}", t_total_mi) = 0;
		allfile << t.traveler_name << ',' << fstr;
		for (Region *region : regions)
		{	auto it = t.active_preview_mileage_by_region.find(region);
			if (it != t.active_preview_mileage_by_region.end())
			{	*fmt::format_to(fstr, "{:.2f}", it->second) = 0;
				allfile << ',' << fstr;
			}
			else	allfile << ",0";
		}
		allfile << '\n';
	}
	*fmt::format_to(fstr, "TOTAL,{:.2f}", total_mi) = 0;
	allfile << fstr;
	for (Region *region : regions)
	{	*fmt::format_to(fstr, ",{:.2f}", region->active_preview_mileage) = 0;
		allfile << fstr;
	}
	allfile << '\n';
	allfile.close();

#ifdef threading_enabled
	if (mtx)
	StatsCsvThread(1, mtx);
#endif
}
