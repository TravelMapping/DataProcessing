#include "WaypointQuadtree.h"
#include "../ErrorList/ErrorList.h"
#include "../HighwaySystem/HighwaySystem.h"
#include "../Route/Route.h"
#include "../Waypoint/Waypoint.h"
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
		   // deleted on termination of program
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
							     // deleted on termination of program
					other_w->colocated->push_back(other_w);
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

Waypoint *WaypointQuadtree::waypoint_at_same_point(Waypoint *w)
{	// find an existing waypoint at the same coordinates as w
	if (refined())
		if (w->lat < mid_lat)
			if (w->lng < mid_lng)
				return sw_child->waypoint_at_same_point(w);
			else	return se_child->waypoint_at_same_point(w);
		else	if (w->lng < mid_lng)
				return nw_child->waypoint_at_same_point(w);
			else	return ne_child->waypoint_at_same_point(w);
	for (Waypoint *p : points)
		if (p->same_coords(w)) return p;
	return 0;
}

std::forward_list<Waypoint*> WaypointQuadtree::near_miss_waypoints(Waypoint *w, double tolerance)
{	// compute and return a list of existing waypoints which are
	// within the near-miss tolerance (in degrees lat, lng) of w
	std::forward_list<Waypoint*> near_miss_points;

	//std::cout << "DEBUG: computing nmps for " << w->str() << " within " << std::to_string(tolerance) << " in " << str() << std::endl;
	// first check if this is a terminal quadrant, and if it is,
	// we search for NMPs within this quadrant
	if (!refined())
	{	//std::cout << "DEBUG: terminal quadrant comparing with " << points.size() << " points." << std::endl;
		for (Waypoint *p : points)
		  if (p != w && !p->same_coords(w) && p->nearby(w, tolerance))
		  {	//std::cout << "DEBUG: found nmp " << p->str() << std::endl;
			near_miss_points.push_front(p);
		  }
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
		if (look_north && look_west) near_miss_points.splice_after(near_miss_points.before_begin(), nw_child->near_miss_waypoints(w, tolerance));
		if (look_north && look_east) near_miss_points.splice_after(near_miss_points.before_begin(), ne_child->near_miss_waypoints(w, tolerance));
		if (look_south && look_west) near_miss_points.splice_after(near_miss_points.before_begin(), sw_child->near_miss_waypoints(w, tolerance));
		if (look_south && look_east) near_miss_points.splice_after(near_miss_points.before_begin(), se_child->near_miss_waypoints(w, tolerance));
	     }
	return near_miss_points;
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
	     {	// skip if this point is occupied by only waypoints in devel systems
		if (!w->is_or_colocated_with_active_or_preview()) continue;
		// skip if colocated and not at front of list
		if (w->colocated && w != w->colocated->front()) continue;
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

unsigned int WaypointQuadtree::max_colocated()
{	// return the maximum number of waypoints colocated at any one location
	unsigned int max_col = 1;
	for (Waypoint *p : point_list())
	  if (max_col < p->num_colocated()) max_col = p->num_colocated();
	//std::cout << "Largest colocate count = " << max_col << std::endl;
	return max_col;
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

#ifdef threading_enabled

void WaypointQuadtree::terminal_nodes(std::forward_list<WaypointQuadtree*>* nodes, size_t& slot, int& size)
{	// add to an array of interleaved lists of terminal nodes
	// for the multi-threaded WaypointQuadtree::sort function
	if (refined())
	     {	ne_child->terminal_nodes(nodes, slot, size);
		nw_child->terminal_nodes(nodes, slot, size);
		se_child->terminal_nodes(nodes, slot, size);
		sw_child->terminal_nodes(nodes, slot, size);
	     }
	else {	nodes[slot].push_front(this);
		slot = (slot+1) % size;
	     }
}

void WaypointQuadtree::sort(int numthreads)
{	std::forward_list<WaypointQuadtree*>* nodes = new std::forward_list<WaypointQuadtree*>[numthreads];
	size_t slot = 0;
	terminal_nodes(nodes, slot, numthreads);

	std::vector<std::thread> thr(numthreads);
	for (int t = 0; t < numthreads; t++)	thr[t] = std::thread(sortnodes, nodes+t);
	for (int t = 0; t < numthreads; t++)	thr[t].join();
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
