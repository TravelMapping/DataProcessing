#include "Route.h"
#include "../Args/Args.h"
#include "../Datacheck/Datacheck.h"
#include "../ErrorList/ErrorList.h"
#include "../HighwaySegment/HighwaySegment.h"
#include "../HighwaySystem/HighwaySystem.h"
#include "../Waypoint/Waypoint.h"
#include "../WaypointQuadtree/WaypointQuadtree.h"
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
	char* c = wptdata;
	while (*c == '\n' || *c == '\r') c++;	// skip leading blank lines
	for (size_t spn = 0; *c; c += spn)
	{	for (spn = strcspn(c, "\n\r"); c[spn] == '\n' || c[spn] == '\r'; spn++) c[spn] = 0;
		lines.emplace_back(c);
	}

	// process lines
	const size_t linecount = lines.size();
	Waypoint *w = points.alloc(linecount);
	HighwaySegment* s = segments.alloc(linecount ? linecount-1 : 0); // cope with zero-waypoint files: all blank lines, not even any whitespace
	lines.push_back(wptdata+wptdatasize+1);	// add a dummy "past-the-end" element to make l[1]-2 work
	for (char **l = lines.data(), **dummy = l+linecount; l < dummy; l++)
	{	// strip whitespace from beginning...
		#define SKIP {--points.size; if (segments.size) /*cope with all-whitespace files*/ --segments.size; continue;}
		while (**l == ' ' || **l == '\t') (*l)++;
		if (!**l) SKIP			// whitespace-only line; skip
		// ...and from end		// first, seek to end of this line from beginning of next
		char* e = l[1]-2;		// -2 skips over the 0 inserted while splitting wptdata into lines
		while (*e == 0) e--;		// skip back more for CRLF cases, and lines followed by blank lines
		while (*e == ' ' || *e == '\t') *e-- = 0;
		try {new(w) Waypoint(*l, this);}
		     // placement new
		catch (const int) SKIP
		#undef SKIP

	      #ifndef threading_enabled
		// look for near-miss points (before we add this one in)
		all_waypoints->near_miss_waypoints(w, 0.0005);
		for (Waypoint *other_w : w->near_miss_points) other_w->near_miss_points.push_front(w);
	      #endif

		all_waypoints->insert(w, 1);

		// single-point Datachecks, and HighwaySegment
		w->out_of_bounds(fstr);
		w->label_invalid_char();
		if (w > points.data)
		{	// add HighwaySegment, if not first point
			new(s) HighwaySegment(w, this);
			// placement new
			// visible distance update, and last segment length check
			double last_distance = s->length;
			vis_dist += last_distance;
			if (last_distance > 20)
			{	sprintf(fstr, "%.2f", last_distance);
				Datacheck::add(this, w[-1].label, w->label, "", "LONG_SEGMENT", fstr);
			}
			s++;
		}
		else if (w->is_hidden) // look for hidden beginning
		     {	Datacheck::add(this, w->label, "", "", "HIDDEN_TERMINUS", "");
			last_visible = w;
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
		++w;
	}
	delete[] wptdata;

	// per-route datachecks
	if (points.size < 2) el->add_error("Route contains fewer than 2 points: " + str());
	else {	// look for hidden endpoint
		if (points.back().is_hidden)
		{	Datacheck::add(this, points.back().label, "", "", "HIDDEN_TERMINUS", "");
			// do one last check in case a VISIBLE_DISTANCE error coexists
			// here, as this was only checked earlier for visible points
			points.back().visible_distance(fstr, vis_dist, last_visible);
		}
		// angle check is easier with a traditional for loop and array indices
		for (Waypoint* p = points.data+1; p < points.end()-1; p++)
		{	//cout << "computing angle for " << p[-1].str() << ' ' << p->str() << ' ' << p[1].str() << endl;
			if (p[-1].same_coords(p) || p[1].same_coords(p))
				Datacheck::add(this, p[-1].label, p->label, p[1].label, "BAD_ANGLE", "");
			else {	double angle = p->angle();
				if (angle > 135)
				{	sprintf(fstr, "%.2f", angle);
					Datacheck::add(this, p[-1].label, p->label, p[1].label, "SHARP_ANGLE", fstr);
				}
			     }
		}
	     }
	//std::cout << '.' << std::flush;
	//std::cout << str() << std::flush;
	//print_route();
}
