#include "WaypointQuadtree.h"
#include "../Args/Args.h"
#include "../Datacheck/Datacheck.h"
#include "../ErrorList/ErrorList.h"
#include "../HighwaySystem/HighwaySystem.h"
#include "../Route/Route.h"
#include "../Waypoint/Waypoint.h"
#include <cstring>
#ifdef threading_enabled
#include <thread>
#endif
inline bool WaypointQuadtree::WaypointQuadtree::refined()
{	return nw_child;
}

WaypointQuadtree::WaypointQuadtree(double MinLat, double MinLng, double MaxLat, double MaxLng)
{	// initialize an empty quadtree node on a given space"""
	min_lat = MinLat;
	min_lng = MinLng;
	max_lat = MaxLat;
	max_lng = MaxLng;
	mid_lat = (min_lat + max_lat) / 2;
	mid_lng = (min_lng + max_lng) / 2;
	nw_child = 0;
	ne_child = 0;
	sw_child = 0;
	se_child = 0;
	unique_locations = 0;
}

void WaypointQuadtree::refine()
{	// refine a quadtree into 4 sub-quadrants"""
	//std::cout << "QTDEBUG: " << str() << " being refined" << std::endl;
	nw_child = new WaypointQuadtree(mid_lat, min_lng, max_lat, mid_lng);
	ne_child = new WaypointQuadtree(mid_lat, mid_lng, max_lat, max_lng);
	sw_child = new WaypointQuadtree(min_lat, min_lng, mid_lat, mid_lng);
	se_child = new WaypointQuadtree(min_lat, mid_lng, mid_lat, max_lng);
		   // deleted by final_report
	for (Waypoint *p : points) insert(p, 0);
	points.clear();
}

void WaypointQuadtree::insert(Waypoint *w, bool init)
{	// insert Waypoint *w into this quadtree node
	//std::cout << "QTDEBUG: " << str() << " insert " << w->str() << std::endl;
	mtx.lock();
	if (!refined())
	     {	// look for colocated points during initial insertion
		if (init)
		{	Waypoint *other_w = 0;
			for (Waypoint *p : points)
			  if (p->same_coords(w))
			  {	other_w = p;
				break;
			  }
			if (other_w)
			{	// see if this is the first point colocated with other_w
				if (!other_w->colocated)
				{	other_w->colocated = new std::list<Waypoint*>;
							     // deleted by final_report
					other_w->colocated->push_back(other_w);
				}
				// DUPLICATE_COORDS datacheck
				for (Waypoint* p : *other_w->colocated)
				  if (p->route == w->route)
				  {	char s[48]; int
					e=sprintf(s,"(%.15g",w->lat); if (int(w->lat)==w->lat) strcpy(s+e, ".0"); std::string info(s);
					e=sprintf(s,",%.15g",w->lng); if (int(w->lng)==w->lng) strcpy(s+e, ".0"); info += s;
					Datacheck::add(w->route, p->label, w->label, "", "DUPLICATE_COORDS", info+')');
				  }
				other_w->colocated->push_back(w);
				w->colocated = other_w->colocated;
			}
		}
		if (!w->colocated || w == w->colocated->front())
		{	//std::cout << "QTDEBUG: " << str() << " at " << unique_locations << " unique locations" << std::endl;
			unique_locations++;
		}
		points.push_back(w);
		if (unique_locations > 50)  // 50 unique points max per quadtree node
			refine();
		mtx.unlock();
	     }
	else {	mtx.unlock();
		if (w->lat < mid_lat)
			if (w->lng < mid_lng)
				sw_child->insert(w, init);
			else	se_child->insert(w, init);
		else	if (w->lng < mid_lng)
				nw_child->insert(w, init);
			else	ne_child->insert(w, init);
	     }
}

void WaypointQuadtree::near_miss_waypoints(Waypoint *w, double tolerance)
{	// compute a list of existing waypoints within the
	// near-miss tolerance (in degrees lat, lng) of w

	// first check if this is a terminal quadrant, and if it is,
	// we search for NMPs within this quadrant
	if (!refined())
	{	for (Waypoint *p : points)
		  if (// iff threading_enabled, w is already in quadtree
		      #ifdef threading_enabled
			p != w &&
		      #endif
			p->nearby(w, tolerance) && !p->same_coords(w)
		     )	w->near_miss_points.push_front(p);
	}
	// if we're not a terminal quadrant, we need to determine which
	// of our child quadrants we need to search and recurse into
	// each
	else {	//std::cout << "DEBUG: recursive case, mid_lat=" << std::to_string(mid_lat) << " mid_lng=" << std::to_string(mid_lng) << std::endl;
		bool look_north = (w->lat + tolerance) >= mid_lat;
		bool look_south = (w->lat - tolerance) <= mid_lat;
		bool look_east = (w->lng + tolerance) >= mid_lng;
		bool look_west = (w->lng - tolerance) <= mid_lng;
		//std::cout << "DEBUG: recursive case, " << look_north << " " << look_south << " " << look_east << " " << look_west << std::endl;
		// now look in the appropriate child quadrants
		if (look_north && look_west) nw_child->near_miss_waypoints(w, tolerance);
		if (look_north && look_east) ne_child->near_miss_waypoints(w, tolerance);
		if (look_south && look_west) sw_child->near_miss_waypoints(w, tolerance);
		if (look_south && look_east) se_child->near_miss_waypoints(w, tolerance);
	     }
}

std::string WaypointQuadtree::str()
{	char s[139];
	sprintf(s, "WaypointQuadtree at (%.15g,%.15g) to (%.15g,%.15g)", min_lat, min_lng, max_lat, max_lng);
	if (refined())
		return std::string(s) + " REFINED";
	else	return std::string(s) + " contains " + std::to_string(points.size()) + " waypoints";
}

unsigned int WaypointQuadtree::size()
{	// return the number of Waypoints in the tree
	if (refined())
		return nw_child->size() + ne_child->size() + sw_child->size() + se_child->size();
	else	return points.size();
}

std::list<Waypoint*> WaypointQuadtree::point_list()
{	// return a list of all points in the quadtree
	if (refined())
	{	std::list<Waypoint*> all_points;
		all_points.splice(all_points.end(), ne_child->point_list());
		all_points.splice(all_points.end(), nw_child->point_list());
		all_points.splice(all_points.end(), se_child->point_list());
		all_points.splice(all_points.end(), sw_child->point_list());
		return all_points;
	}
	else	return points;
}

void WaypointQuadtree::graph_points(std::vector<Waypoint*>& hi_priority_points, std::vector<Waypoint*>& lo_priority_points)
{	// return a list of points to be used as indices to HighwayGraph vertices
	if (refined())
	     {	ne_child->graph_points(hi_priority_points, lo_priority_points);
		nw_child->graph_points(hi_priority_points, lo_priority_points);
		se_child->graph_points(hi_priority_points, lo_priority_points);
		sw_child->graph_points(hi_priority_points, lo_priority_points);
	     }
	else for (Waypoint *w : points)
	     {	if (w->colocated)
		{	// skip if not at front of list
			if (w != w->colocated->front()) continue;
			// VISIBLE_HIDDEN_COLOC datacheck
			for (auto p = ++w->colocated->begin(); p != w->colocated->end(); p++)
			  if ((*p)->is_hidden != w->colocated->front()->is_hidden)
			  {	if ((*p)->is_hidden)
					Datacheck::add(w->route, w->label, "", "", "VISIBLE_HIDDEN_COLOC",
						       (*p)->route->root+"@"+(*p)->label);
				else	Datacheck::add((*p)->route, (*p)->label, "", "", "VISIBLE_HIDDEN_COLOC",
						       w->route->root+"@"+w->label);
				break;
			  }
		}
		// skip if this point is occupied by only waypoints in devel systems
		if (!w->is_or_colocated_with_active_or_preview()) continue;
		// store a colocated list with any devel system entries removed
		if (!w->colocated) w->ap_coloc.push_back(w);
		else for (Waypoint *p : *(w->colocated))
		  if (p->route->system->active_or_preview())
		    w->ap_coloc.push_back(p);
		// determine vertex name simplification priority
		if (	w->ap_coloc.size() != 2
		     || w->ap_coloc.front()->route->abbrev.size()
		     || w->ap_coloc.back()->route->abbrev.size()
		   )	lo_priority_points.push_back(w);
		else	hi_priority_points.push_back(w);
	     }
}

bool WaypointQuadtree::is_valid(ErrorList &el)
{	// make sure the quadtree is valid
	if (refined())
	{	// refined nodes should not contain points
		if (!points.empty()) el.add_error(str() + " contains " + std::to_string(points.size()) + "waypoints");
		return nw_child->is_valid(el) && ne_child->is_valid(el) && sw_child->is_valid(el) && se_child->is_valid(el);
		// EDB: Removed tests for whether a node has children.
		// This made more sense in the original Python version of the code.
		// There, the criterion for whether a node was refined was whether points was None, or an empty list.
		// Static typing in C++ doesn't allow this, thus the refined() test becomes whether a node has children, making this sanity check moot.
	}
	else	// not refined, but should have no more than 50 unique points
	  if (unique_locations > 50)
	  {	el.add_error("WaypointQuadtree.is_valid terminal quadrant has too many unique points (" + std::to_string(unique_locations) + ")");
		return 0;
	  }
	std::cout << "WaypointQuadtree is valid." << std::endl;
	return 1;
}

unsigned int WaypointQuadtree::total_nodes()
{	if (!refined())
		// not refined, no children, return 1 for self
		return 1;
	else	return 1 + nw_child->total_nodes() + ne_child->total_nodes() + sw_child->total_nodes() + se_child->total_nodes();
}

void WaypointQuadtree::get_tmg_lines(std::list<std::string> &vertices, std::list<std::string> &edges, std::string n_name)
{	if (refined())
	{	double cmn_lat = min_lat;
		double cmx_lat = max_lat;
		if (cmn_lat < -80)	cmn_lat = -80;
		if (cmx_lat > 80)	cmx_lat = 80;
		edges.push_back(std::to_string(vertices.size())+" "+std::to_string(vertices.size()+1)+" "+n_name+"_NS");
		edges.push_back(std::to_string(vertices.size()+2)+" "+std::to_string(vertices.size()+3)+" "+n_name+"_EW");
		vertices.push_back(n_name+"@+S "+std::to_string(cmn_lat)+" "+std::to_string(mid_lng));
		vertices.push_back(n_name+"@+N "+std::to_string(cmx_lat)+" "+std::to_string(mid_lng));
		vertices.push_back(n_name+"@+W "+std::to_string(mid_lat)+" "+std::to_string(min_lng));
		vertices.push_back(n_name+"@+E "+std::to_string(mid_lat)+" "+std::to_string(max_lng));
		nw_child->get_tmg_lines(vertices, edges, n_name+"A");
		ne_child->get_tmg_lines(vertices, edges, n_name+"B");
		sw_child->get_tmg_lines(vertices, edges, n_name+"C");
		se_child->get_tmg_lines(vertices, edges, n_name+"D");
	}
}

void WaypointQuadtree::write_qt_tmg(std::string filename)
{	std::list<std::string> vertices, edges;
	get_tmg_lines(vertices, edges, "M");
	std::ofstream tmgfile(filename);
	tmgfile << "TMG 1.0 simple\n";
	tmgfile << vertices.size() << ' ' << edges.size() << '\n';
	for (std::string& v : vertices)	tmgfile << v << '\n';
	for (std::string& e : edges)	tmgfile << e << '\n';
	tmgfile.close();
}

void WaypointQuadtree::final_report(std::vector<unsigned int>& colocate_counts)
{	// gather & print info for final colocation stats report,
	// in the process deleting nodes, waypoints & coloc lists
	if (refined())
	     {	ne_child->final_report(colocate_counts); delete ne_child;
		nw_child->final_report(colocate_counts); delete nw_child;
		se_child->final_report(colocate_counts); delete se_child;
		sw_child->final_report(colocate_counts); delete sw_child;
	     }
	else for (Waypoint *w : points)
	     {	if (!w->colocated) colocate_counts[1] +=1;
		else if (w == w->colocated->front())
		{   while (w->colocated->size() >= colocate_counts.size()) colocate_counts.push_back(0);
		    colocate_counts[w->colocated->size()] += 1;
		    if (w->colocated->size() >= 9 && !Args::errorcheck)
		    {	printf("(%.15g, %.15g) is occupied by %i waypoints: ['", w->lat, w->lng, (unsigned int)w->colocated->size());
			std::list<Waypoint*>::iterator p = w->colocated->begin();
			std::cout << (*p)->route->root << ' ' << (*p)->label << '\'';
			for (p++; p != w->colocated->end(); p++)
				std::cout << ", '" << (*p)->route->root << ' ' << (*p)->label << '\'';
			std::cout << "]\n";
		    }
		}
		else if (w == w->colocated->back()) delete w->colocated;
		delete w;
	     }
}

#ifdef threading_enabled

void WaypointQuadtree::terminal_nodes(std::forward_list<WaypointQuadtree*>* nodes, size_t& slot)
{	// add to an array of interleaved lists of terminal nodes
	// for the multi-threaded WaypointQuadtree::sort function
	if (refined())
	     {	ne_child->terminal_nodes(nodes, slot);
		nw_child->terminal_nodes(nodes, slot);
		se_child->terminal_nodes(nodes, slot);
		sw_child->terminal_nodes(nodes, slot);
	     }
	else {	nodes[slot].push_front(this);
		slot = (slot+1) % Args::numthreads;
	     }
}

void WaypointQuadtree::sort()
{	std::forward_list<WaypointQuadtree*>* nodes = new std::forward_list<WaypointQuadtree*>[Args::numthreads];
						      // deleted @ end of this function
	size_t slot = 0;
	terminal_nodes(nodes, slot);

	std::vector<std::thread> thr(Args::numthreads);
	for (int t = 0; t < Args::numthreads; t++)	thr[t] = std::thread(sortnodes, nodes+t);
	for (int t = 0; t < Args::numthreads; t++)	thr[t].join();
	delete[] nodes;
}

void WaypointQuadtree::sortnodes(std::forward_list<WaypointQuadtree*>* nodes)
{	for (WaypointQuadtree* node : *nodes)
	{	node->points.sort(sort_root_at_label);
		for (Waypoint *w : node->points)
		  if (w->colocated && w == w->colocated->front()) w->colocated->sort(sort_root_at_label);
	}
}

#else

void WaypointQuadtree::sort()
{	if (refined())
	     {	ne_child->sort();
		nw_child->sort();
		se_child->sort();
		sw_child->sort();
	     }
	else {	points.sort(sort_root_at_label);
		for (Waypoint *w : points)
		  if (w->colocated && w == w->colocated->front()) w->colocated->sort(sort_root_at_label);
	     }
}

#endif
