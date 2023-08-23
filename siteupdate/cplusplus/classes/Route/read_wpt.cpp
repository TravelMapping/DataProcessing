#include "Route.h"
#include "../Args/Args.h"
#include "../Datacheck/Datacheck.h"
#include "../ErrorList/ErrorList.h"
#include "../HighwaySegment/HighwaySegment.h"
#include "../HighwaySystem/HighwaySystem.h"
#include "../Waypoint/Waypoint.h"
#include "../WaypointQuadtree/WaypointQuadtree.h"
#include <cstring>
#include <fstream>

void Route::read_wpt(WaypointQuadtree *all_waypoints, ErrorList *el, bool usa_flag)
{	/* read data into the Route's waypoint list from a .wpt file */
	std::string filename = Args::datapath + "/data/" + rg_str + "/" + system->systemname + "/" + root + ".wpt";
	Waypoint *last_visible;
	double vis_dist = 0;
	char fstr[112];

	// remove full path from all_wpt_files list
	awf_mtx.lock();
	all_wpt_files.erase(filename);
	awf_mtx.unlock();

	// read .wpt file into memory
	std::ifstream file(filename);
	if (!file)
	{	el->add_error("[Errno 2] No such file or directory: '" + filename + '\'');
		file.close();
		return;
	}
	file.seekg(0, std::ios::end);
	unsigned long wptdatasize = file.tellg();
	file.seekg(0, std::ios::beg);
	char *wptdata = new char[wptdatasize+1];
			// deleted after processing lines
	file.read(wptdata, wptdatasize);
	wptdata[wptdatasize] = 0; // add null terminator
	file.close();

	// split file into lines
	std::vector<char*> lines;
	size_t spn = 0;
	for (char* c = wptdata; *c; c += spn)
	{	for (spn = strcspn(c, "\n\r"); c[spn] == '\n' || c[spn] == '\r'; spn++) c[spn] = 0;
		lines.emplace_back(c);
	}
	if (lines.empty())
	{	delete[] wptdata;
		el->add_error(filename + " is empty or begins with null zero");
		return;
	}
	lines.push_back(wptdata+wptdatasize+1);		// add a dummy "past-the-end" element to make lines[l+1]-2 work

	// process lines
	for (unsigned int l = lines[1] < wptdata+2; l < lines.size()-1; l++)
	{	// strip whitespace from end...
		char* endchar = lines[l+1]-2;		// -2 skips over the 0 inserted while splitting wptdata into lines
		while (*endchar == 0) endchar--;	// skip back more for CRLF cases, and lines followed by blank lines
		if (endchar < lines[l]) continue;	// line is blank; skip (avoid seeking before BOF)
		while (*endchar == ' ' || *endchar == '\t')
		{	*endchar = 0;
			endchar--;
		} // ...and from beginning
		while (lines[l][0] == ' ' || lines[l][0] == '\t') lines[l]++;
		if (!*lines[l]) continue;		// whitespace-only line; skip
		Waypoint *w = new Waypoint(lines[l], this);
			      // deleted by WaypointQuadtree::final_report, or immediately below if invalid
		// lat & lng both equal to 0 marks a point as invalid
		if (!w->lat && !w->lng)
		{	delete w;
			continue;
		}
		point_list.push_back(w);

	      #ifndef threading_enabled
		// look for near-miss points (before we add this one in)
		all_waypoints->near_miss_waypoints(w, 0.0005);
		for (Waypoint *other_w : w->near_miss_points) other_w->near_miss_points.push_front(w);
	      #endif

		all_waypoints->insert(w, 1);

		// single-point Datachecks, and HighwaySegment
		w->out_of_bounds(fstr);
		w->label_invalid_char();
		if (point_list.size() > 1)
		{	w->distance_update(fstr, vis_dist, point_list[point_list.size()-2]);
			// add HighwaySegment, if not first point
			segment_list.push_back(new HighwaySegment(point_list[point_list.size()-2], w, this));
					       // deleted on termination of program
		}
		else if (point_list[0]->is_hidden) // look for hidden beginning
		     {	Datacheck::add(this, point_list[0]->label, "", "", "HIDDEN_TERMINUS", "");
			last_visible = point_list[0];
		     }
		// checks for visible points
		if (!w->is_hidden)
		{	const char *slash = strchr(w->label.data(), '/');
			if (usa_flag && w->label.size() >= 2)
			{	w->bus_with_i();
				w->interstate_no_hyphen();
				w->us_letter();
			}
			w->label_invalid_ends();
			w->label_looks_hidden();
			w->label_lowercase();
			w->label_parens();
			w->label_slashes(slash);
			w->lacks_generic();
			w->underscore_datachecks(slash);
			w->visible_distance(fstr, vis_dist, last_visible);
		}
	}
	delete[] wptdata;

	// per-route datachecks
	if (point_list.size() < 2) el->add_error("Route contains fewer than 2 points: " + str());
	else {	// look for hidden endpoint
		if (point_list.back()->is_hidden)
		{	Datacheck::add(this, point_list.back()->label, "", "", "HIDDEN_TERMINUS", "");
			// do one last check in case a VISIBLE_DISTANCE error coexists
			// here, as this was only checked earlier for visible points
			point_list.back()->visible_distance(fstr, vis_dist, last_visible);
		}
		// angle check is easier with a traditional for loop and array indices
		for (unsigned int i = 1; i < point_list.size()-1; i++)
		{	//cout << "computing angle for " << point_list[i-1].str() << ' ' << point_list[i].str() << ' ' << point_list[i+1].str() << endl;
			if (point_list[i-1]->same_coords(point_list[i]) || point_list[i+1]->same_coords(point_list[i]))
				Datacheck::add(this, point_list[i-1]->label, point_list[i]->label, point_list[i+1]->label, "BAD_ANGLE", "");
			else {	double angle = point_list[i]->angle(point_list[i-1], point_list[i+1]);
				if (angle > 135)
				{	sprintf(fstr, "%.2f", angle);
					Datacheck::add(this, point_list[i-1]->label, point_list[i]->label, point_list[i+1]->label, "SHARP_ANGLE", fstr);
				}
			     }
		}
	     }
	//std::cout << '.' << std::flush;
	//std::cout << str() << std::flush;
	//print_route();
}
