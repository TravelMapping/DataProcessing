#include "../classes/Args/Args.h"
#include "../classes/HighwaySystem/HighwaySystem.h"
#include "../classes/Route/Route.h"
#include <fstream>
#include <string>

void route_and_label_logs(time_t* timestamp)
{	unsigned int total_unused_alt_labels = 0;
	unsigned int total_unusedaltroutenames = 0;
	std::list<std::string> unused_alt_labels;
	std::ofstream piufile(Args::logfilepath+"/pointsinuse.log");
	std::ofstream lniufile(Args::logfilepath+"/listnamesinuse.log");
	std::ofstream uarnfile(Args::logfilepath+"/unusedaltroutenames.log");
	std::ofstream flipfile(Args::logfilepath+"/flippedroutes.log");
	*timestamp = time(0);
	piufile << "Log file created at: " << ctime(timestamp);
	lniufile << "Log file created at: " << ctime(timestamp);
	uarnfile << "Log file created at: " << ctime(timestamp);
	for (HighwaySystem& h : HighwaySystem::syslist)
	{	for (Route& r : h.routes)
		{	// labelsinuse.log line
			if (r.labels_in_use.size())
			{	piufile << r.root << '(' << r.points.size << "):";
				std::list<std::string> liu_list(r.labels_in_use.begin(), r.labels_in_use.end());
				liu_list.sort();
				for (std::string& label : liu_list) piufile << ' ' << label;
				piufile << '\n';
				r.labels_in_use.clear();
			}
			// unusedaltlabels.log lines, to be sorted by root later
			if (r.unused_alt_labels.size())
			{	total_unused_alt_labels += r.unused_alt_labels.size();
				std::string ual_entry = r.root + '(' + std::to_string(r.unused_alt_labels.size()) + "):";
				std::list<std::string> ual_list(r.unused_alt_labels.begin(), r.unused_alt_labels.end());
				ual_list.sort();
				for (std::string& label : ual_list) ual_entry += ' ' + label;
				unused_alt_labels.push_back(ual_entry);
				r.unused_alt_labels.clear();
			}
			// flippedroutes.log line
			if (r.is_reversed()) flipfile << r.root << '\n';
		}
		// listnamesinuse.log line
		if (h.listnamesinuse.size())
		{	lniufile << h.systemname << '(' << h.routes.size << "):";
			std::list<std::string> lniu_list(h.listnamesinuse.begin(), h.listnamesinuse.end());
			lniu_list.sort();
			for (std::string& list_name : lniu_list) lniufile << " \"" << list_name << '"';
			lniufile << '\n';
			h.listnamesinuse.clear();
		}
		// unusedaltroutenames.log line
		if (h.unusedaltroutenames.size())
		{	total_unusedaltroutenames += h.unusedaltroutenames.size();
			uarnfile << h.systemname << '(' << h.unusedaltroutenames.size() << "):";
			std::list<std::string> uarn_list(h.unusedaltroutenames.begin(), h.unusedaltroutenames.end());
			uarn_list.sort();
			for (std::string& list_name : uarn_list) uarnfile << " \"" << list_name << '"';
			uarnfile << '\n';
			h.unusedaltroutenames.clear();
		}
	}
	piufile.close();
	lniufile.close();
	flipfile.close();
	uarnfile << "Total: " << total_unusedaltroutenames << '\n';
	uarnfile.close();
	// sort lines and write unusedaltlabels.log
	unused_alt_labels.sort();
	std::ofstream ualfile(Args::logfilepath+"/unusedaltlabels.log");
	*timestamp = time(0);
	ualfile << "Log file created at: " << ctime(timestamp);
	for (std::string &ual_entry : unused_alt_labels) ualfile << ual_entry << '\n';
	unused_alt_labels.clear();
	ualfile << "Total: " << total_unused_alt_labels << '\n';
	ualfile.close();
}
