#!/usr/bin/env python3
# Travel Mapping Project, Jim Teresco, 2015-2018
"""Python code to read .csv and .wpt files and prepare for
adding to the Travel Mapping Project database.

(c) 2015-2018, Jim Teresco

This module defines classes to represent the contents of a
.csv file that lists the highways within a system, and a
.wpt file that lists the waypoints for a given highway.
"""

import argparse
import datetime
import math
import os
import re
import sys
import time
import threading

class ElapsedTime:
    """To get a nicely-formatted elapsed time string for printing"""

    def __init__(self):
        self.start_time = time.time()

    def et(self):
        return "[{0:.1f}] ".format(time.time()-self.start_time)

class ErrorList:
    """Track a list of potentially fatal errors"""

    def __init__(self):
        self.lock = threading.Lock()
        self.error_list = []

    def add_error(self, e):
        self.lock.acquire()
        print("ERROR: " + e)
        self.error_list.append(e)
        self.lock.release()

class WaypointQuadtree:
    """This class defines a recursive quadtree structure to store
    Waypoint objects for efficient geometric searching.
    """
    def __init__(self,min_lat,min_lng,max_lat,max_lng):
        """initialize an empty quadtree node on a given space"""
        self.min_lat = min_lat
        self.min_lng = min_lng
        self.max_lat = max_lat
        self.max_lng = max_lng
        self.mid_lat = (self.min_lat + self.max_lat) / 2
        self.mid_lng = (self.min_lng + self.max_lng) / 2
        self.nw_child = None
        self.ne_child = None
        self.sw_child = None
        self.se_child = None
        self.points = []
        self.unique_locations = 0

    def refine(self):
        """refine a quadtree into 4 sub-quadrants"""
        #print("QTDEBUG: " + str(self) + " being refined")
        self.nw_child = WaypointQuadtree(self.mid_lat,self.min_lng,self.max_lat,self.mid_lng)
        self.ne_child = WaypointQuadtree(self.mid_lat,self.mid_lng,self.max_lat,self.max_lng)
        self.sw_child = WaypointQuadtree(self.min_lat,self.min_lng,self.mid_lat,self.mid_lng)
        self.se_child = WaypointQuadtree(self.min_lat,self.mid_lng,self.mid_lat,self.max_lng)
        points = self.points
        self.points = None
        for p in points:
            self.insert(p)

    def insert(self,w):
        """insert Waypoint w into this quadtree node"""
        #print("QTDEBUG: " + str(self) + " insert " + str(w))
        if self.points is not None:
            if self.waypoint_at_same_point(w) is None:
                #print("QTDEBUG: " + str(self) + " at " + str(self.unique_locations) + " unique locations")
                self.unique_locations += 1
            self.points.append(w)
            if self.unique_locations > 50:  # 50 unique points max per quadtree node
                self.refine()
        else:
            if w.lat < self.mid_lat:
                if w.lng < self.mid_lng:
                    self.sw_child.insert(w)
                else:
                    self.se_child.insert(w)
            else:
                if w.lng < self.mid_lng:
                    self.nw_child.insert(w)
                else:
                    self.ne_child.insert(w)

    def waypoint_at_same_point(self,w):
        """find an existing waypoint at the same coordinates as w"""
        if self.points is not None:
            for p in self.points:
                if p.same_coords(w):
                    return p
            return None
        else:
            if w.lat < self.mid_lat:
                if w.lng < self.mid_lng:
                    return self.sw_child.waypoint_at_same_point(w)
                else:
                    return self.se_child.waypoint_at_same_point(w)
            else:
                if w.lng < self.mid_lng:
                    return self.nw_child.waypoint_at_same_point(w)
                else:
                    return self.ne_child.waypoint_at_same_point(w)

    def near_miss_waypoints(self, w, tolerance):
        """compute and return a list of existing waypoints which are
        within the near-miss tolerance (in degrees lat, lng) of w"""
        near_miss_points = []

        #print("DEBUG: computing nmps for " + str(w) + " within " + str(tolerance) + " in " + str(self))
        # first check if this is a terminal quadrant, and if it is,
        # we search for NMPs within this quadrant
        if self.points is not None:
            #print("DEBUG: terminal quadrant (self.points is not None) comparing with " + str(len(self.points)) + " points.")
            for p in self.points:
                if p != w and not p.same_coords(w) and p.nearby(w, tolerance):
                    #print("DEBUG: found nmp " + str(p))
                    near_miss_points.append(p)

        # if we're not a terminal quadrant, we need to determine which
        # of our child quadrants we need to search and recurse into
        # each
        else:
            #print("DEBUG: recursive case, mid_lat=" + str(self.mid_lat) + " mid_lng=" + str(self.mid_lng))
            look_north = (w.lat + tolerance) >= self.mid_lat
            look_south = (w.lat - tolerance) <= self.mid_lat
            look_east = (w.lng + tolerance) >= self.mid_lng
            look_west = (w.lng - tolerance) <= self.mid_lng
            #print("DEBUG: recursive case, " + str(look_north) + " " + str(look_south) + " " + str(look_east) + " " + str(look_west))
            # now look in the appropriate child quadrants
            if look_north and look_west:
                near_miss_points.extend(self.nw_child.near_miss_waypoints(w, tolerance))
            if look_north and look_east:
                near_miss_points.extend(self.ne_child.near_miss_waypoints(w, tolerance))
            if look_south and look_west:
                near_miss_points.extend(self.sw_child.near_miss_waypoints(w, tolerance))
            if look_south and look_east:
                near_miss_points.extend(self.se_child.near_miss_waypoints(w, tolerance))

        return near_miss_points

    def __str__(self):
        s = "WaypointQuadtree at (" + str(self.min_lat) + "," + \
            str(self.min_lng) + ") to (" + str(self.max_lat) + "," + \
            str(self.max_lng) + ")"
        if self.points is None:
            return s + " REFINED"
        else:
            return s + " contains " + str(len(self.points)) + " waypoints"

    def size(self):
        """return the number of Waypoints in the tree"""
        if self.points is None:
            return self.nw_child.size() + self.ne_child.size() + self.sw_child.size() + self.se_child.size()
        else:
            return len(self.points)

    def point_list(self):
        """return a list of all points in the quadtree"""
        if self.points is None:
            all_points = []
            all_points.extend(self.ne_child.point_list())
            all_points.extend(self.nw_child.point_list())
            all_points.extend(self.se_child.point_list())
            all_points.extend(self.sw_child.point_list())
            return all_points
        else:
            return self.points

    def is_valid(self):
        """make sure the quadtree is valid"""
        if self.points is None:
            # this should be a refined, so should have all 4 children
            if self.nw_child is None:
                print("ERROR: WaypointQuadtree.is_valid refined quadrant has no NW child.")
                return False
            if self.ne_child is None:
                print("ERROR: WaypointQuadtree.is_valid refined quadrant has no NE child.")
                return False
            if self.sw_child is None:
                print("ERROR: WaypointQuadtree.is_valid refined quadrant has no SW child.")
                return False
            if self.se_child is None:
                print("ERROR: WaypointQuadtree.is_valid refined quadrant has no SE child.")
                return False
            return self.nw_child.is_valid() and self.ne_child.is_valid() and self.sw_child.is_valid() and self.se_child.is_valid()

        else:
            # not refined, but should have no more than 50 points
            if self.unique_locations > 50:
                print("ERROR: WaypointQuadtree.is_valid terminal quadrant has too many unique points (" + str(self.unique_locations) + ")")
                return False
            # not refined, so should not have any children
            if self.nw_child is not None:
                print("ERROR: WaypointQuadtree.is_valid terminal quadrant has NW child.")
                return False
            if self.ne_child is not None:
                print("ERROR: WaypointQuadtree.is_valid terminal quadrant has NE child.")
                return False
            if self.sw_child is not None:
                print("ERROR: WaypointQuadtree.is_valid terminal quadrant has SW child.")
                return False
            if self.se_child is not None:
                print("ERROR: WaypointQuadtree.is_valid terminal quadrant has SE child.")
                return False

        return True

    def max_colocated(self):
        """return the maximum number of waypoints colocated at any one location"""
        max_col = 1
        for p in self.point_list():
            if max_col < p.num_colocated():
                max_col = p.num_colocated()
        print("Largest colocate count = " + str(max_col))
        return max_col

    def total_nodes(self):
        if self.points is not None:
            # not refined, no children, return 1 for self
            return 1
        else:
            return 1 + self.nw_child.total_nodes() + self.ne_child.total_nodes() + self.sw_child.total_nodes() + self.se_child.total_nodes()

    def sort(self):
        if self.points is None:
            self.ne_child.sort()
            self.nw_child.sort()
            self.se_child.sort()
            self.sw_child.sort()
        else:
            self.points.sort(key=lambda waypoint: waypoint.route.root + "@" + waypoint.label)

class Waypoint:
    """This class encapsulates the information about a single waypoint
    from a .wpt file.

    The line consists of one or more labels, at most one of which can
    be a "regular" label.  Others are "hidden" labels and must begin with
    a '+'.  Then an OSM URL which encodes the latitude and longitude.

    root is the unique identifier for the route in which this waypoint
    is defined
    """
    def __init__(self,line,route,datacheckerrors):
        """initialize object from a .wpt file line"""
        self.route = route
        parts = line.split()
        self.label = parts[0]
        self.is_hidden = self.label.startswith('+')
        if len(parts) > 2:
            # all except first and last
            self.alt_labels = parts[1:-1]
        else:
            self.alt_labels = []
        # last has the URL, which needs more work to get lat/lng
        url_parts = parts[-1].split('=')
        if len(url_parts) < 3:
            #print("\nWARNING: Malformed URL in " + route.root + ", line: " + line, end="", flush=True)
            datacheckerrors.append(DatacheckEntry(route,[self.label],'MALFORMED_URL', parts[-1]))
            self.lat = 0
            self.lng = 0
            self.colocated = None
            self.near_miss_points = None
            return
        lat_string = url_parts[1].split("&")[0] # chop off "&lon"
        lng_string = url_parts[2].split("&")[0] # chop off possible "&zoom"

        # make sure lat_string is valid
        point_count = 0
        for c in range(len(lat_string)):
            # check for multiple decimal points
            if lat_string[c] == '.':
                point_count += 1
                if point_count > 1:
                    #print("\nWARNING: Malformed URL in " + route.root + ", line: " + line, end="", flush=True)
                    datacheckerrors.append(DatacheckEntry(route,[self.label],'MALFORMED_URL', parts[-1]))
                    lat_string = "0"
                    lng_string = "0"
                    break
            # check for minus sign not at beginning
            if lat_string[c] == '-' and c > 0:
                #print("\nWARNING: Malformed URL in " + route.root + ", line: " + line, end="", flush=True)
                datacheckerrors.append(DatacheckEntry(route,[self.label],'MALFORMED_URL', parts[-1]))
                lat_string = "0"
                lng_string = "0"
                break
            # check for invalid characters
            if lat_string[c] not in "-.0123456789":
                #print("\nWARNING: Malformed URL in " + route.root + ", line: " + line, end="", flush=True)
                datacheckerrors.append(DatacheckEntry(route,[self.label],'MALFORMED_URL', parts[-1]))
                lat_string = "0"
                lng_string = "0"
                break

        # make sure lng_string is valid
        point_count = 0
        for c in range(len(lng_string)):
            # check for multiple decimal points
            if lng_string[c] == '.':
                point_count += 1
                if point_count > 1:
                    #print("\nWARNING: Malformed URL in " + route.root + ", line: " + line, end="", flush=True)
                    datacheckerrors.append(DatacheckEntry(route,[self.label],'MALFORMED_URL', parts[-1]))
                    lat_string = "0"
                    lng_string = "0"
                    break
            # check for minus sign not at beginning
            if lng_string[c] == '-' and c > 0:
                #print("\nWARNING: Malformed URL in " + route.root + ", line: " + line, end="", flush=True)
                datacheckerrors.append(DatacheckEntry(route,[self.label],'MALFORMED_URL', parts[-1]))
                lat_string = "0"
                lng_string = "0"
                break
            # check for invalid characters
            if lng_string[c] not in "-.0123456789":
                #print("\nWARNING: Malformed URL in " + route.root + ", line: " + line, end="", flush=True)
                datacheckerrors.append(DatacheckEntry(route,[self.label],'MALFORMED_URL', parts[-1]))
                lat_string = "0"
                lng_string = "0"
                break

        self.lat = float(lat_string)
        self.lng = float(lng_string)
        # also keep track of a list of colocated waypoints, if any
        self.colocated = None
        # and keep a list of "near-miss points", if any
        self.near_miss_points = None

    def __str__(self):
        ans = self.route.root + " " + self.label
        if len(self.alt_labels) > 0:
            ans = ans + " [alt: " + str(self.alt_labels) + "]"
        ans = ans + " (" + str(self.lat) + "," + str(self.lng) + ")"
        return ans

    def csv_line(self,id):
        """return csv line to insert into a table"""
        return "'" + str(id) + "','" + self.label + "','" + str(self.lat) + "','" + str(self.lng) + "','" + self.route.root + "'"

    def same_coords(self,other):
        """return if this waypoint is colocated with the other,
        using exact lat,lng match"""
        return self.lat == other.lat and self.lng == other.lng

    def nearby(self,other,tolerance):
        """return if this waypoint's coordinates are within the given
        tolerance (in degrees) of the other"""
        return abs(self.lat - other.lat) < tolerance and \
            abs(self.lng - other.lng) < tolerance

    def num_colocated(self):
        """return the number of points colocated with this one (including itself)"""
        if self.colocated is None:
            return 1
        else:
            return len(self.colocated)

    def distance_to(self,other):
        """return the distance in miles between this waypoint and another
        including the factor defined by the CHM project to adjust for
        unplotted curves in routes"""
        # convert to radians
        rlat1 = math.radians(self.lat)
        rlng1 = math.radians(self.lng)
        rlat2 = math.radians(other.lat)
        rlng2 = math.radians(other.lng)

        ans = math.acos(math.cos(rlat1)*math.cos(rlng1)*math.cos(rlat2)*math.cos(rlng2) +\
                        math.cos(rlat1)*math.sin(rlng1)*math.cos(rlat2)*math.sin(rlng2) +\
                        math.sin(rlat1)*math.sin(rlat2)) * 3963.1 # EARTH_RADIUS;
        return ans * 1.02112

    def angle(self,pred,succ):
        """return the angle in degrees formed by the waypoints between the
        line from pred to self and self to succ"""
        # convert to radians
        rlatself = math.radians(self.lat)
        rlngself = math.radians(self.lng)
        rlatpred = math.radians(pred.lat)
        rlngpred = math.radians(pred.lng)
        rlatsucc = math.radians(succ.lat)
        rlngsucc = math.radians(succ.lng)

        x0 = math.cos(rlngpred)*math.cos(rlatpred)
        x1 = math.cos(rlngself)*math.cos(rlatself)
        x2 = math.cos(rlngsucc)*math.cos(rlatsucc)

        y0 = math.sin(rlngpred)*math.cos(rlatpred)
        y1 = math.sin(rlngself)*math.cos(rlatself)
        y2 = math.sin(rlngsucc)*math.cos(rlatsucc)

        z0 = math.sin(rlatpred)
        z1 = math.sin(rlatself)
        z2 = math.sin(rlatsucc)

        return math.degrees(math.acos(((x2 - x1)*(x1 - x0) + (y2 - y1)*(y1 - y0) + (z2 - z1)*(z1 - z0)) / math.sqrt(((x2 - x1)*(x2 - x1) + (y2 - y1)*(y2 - y1) + (z2 - z1)*(z2 - z1)) * ((x1 - x0)*(x1 - x0) + (y1 - y0)*(y1 - y0) + (z1 - z0)*(z1 - z0)))))

    def canonical_waypoint_name(self,log):
        """Best name we can come up with for this point bringing in
        information from itself and colocated points (if active/preview)
        """
        # start with the failsafe name, and see if we can improve before
        # returning
        name = self.simple_waypoint_name()

        # if no colocated points, there's nothing to do - we just use
        # the route@label form and deal with conflicts elsewhere
        if self.colocated is None:
            return name

        # get a colocated list that any devel system entries removed
        colocated = []
        for w in self.colocated:
            if w.route.system.active_or_preview():
                colocated.append(w)
        # just return the simple name if only one active/preview waypoint
        if (len(colocated) == 1):
            return name

        # straightforward concurrency example with matching waypoint
        # labels, use route/route/route@label, except also matches
        # any hidden label
        # TODO: compress when some but not all labels match, such as
        # E127@Kan&AH60@Kan_N&AH64@Kan&AH67@Kan&M38@Kan
        # or possibly just compress ignoring the _ suffixes here
        routes = ""
        pointname = ""
        matches = 0
        for w in colocated:
            if routes == "":
                routes = w.route.list_entry_name()
                pointname = w.label
                matches = 1
            elif pointname == w.label or w.label.startswith('+'):
                # this check seems odd, but avoids double route names
                # at border crossings
                if routes != w.route.list_entry_name():
                    routes += "/" + w.route.list_entry_name()
                matches += 1
        if matches == len(colocated):
            log.append("Straightforward concurrency: " + name + " -> " + routes + "@" + pointname)
            return routes + "@" + pointname

        # straightforward 2-route intersection with matching labels
        # NY30@US20&US20@NY30 would become NY30/US20
        # or
        # 2-route intersection with one or both labels having directional
        # suffixes but otherwise matching route
        # US24@CO21_N&CO21@US24_E would become US24_E/CO21_N

        if len(colocated) == 2:
            w0_list_entry = colocated[0].route.list_entry_name()
            w1_list_entry = colocated[1].route.list_entry_name()
            w0_label = colocated[0].label
            w1_label = colocated[1].label
            if (w0_list_entry == w1_label or \
                w1_label.startswith(w0_list_entry + '_')) and \
                (w1_list_entry == w0_label or \
                w0_label.startswith(w1_list_entry + '_')):
                log.append("Straightforward intersection: " + name + " -> " + w1_label + '/' + w0_label)
                return w1_label + '/' + w0_label

        # check for cases like
        # I-10@753B&US90@I-10(753B)
        # which becomes
        # I-10(753B)/US90
        # more generally,
        # I-30@135&US67@I-30(135)&US70@I-30(135)
        # becomes
        # I-30(135)/US67/US70
        # but also matches some other cases that perhaps should
        # be checked or handled separately, though seems OK
        # US20@NY30A&NY30A@US20&NY162@US20
        # becomes
        # US20/NY30A/NY162

        for match_index in range(0,len(colocated)):
            lookfor1 = colocated[match_index].route.list_entry_name()
            lookfor2 = colocated[match_index].route.list_entry_name() + \
               '(' + colocated[match_index].label + ')'
            all_match = True
            for check_index in range(0,len(colocated)):
                if match_index == check_index:
                    continue
                if (colocated[check_index].label != lookfor1) and \
                   (colocated[check_index].label != lookfor2):
                    all_match = False
            if all_match:
                if (colocated[match_index].label[0:1].isnumeric()):
                    label = lookfor2
                else:
                    label = lookfor1
                for add_index in range(0,len(colocated)):
                    if match_index == add_index:
                        continue
                    label += '/' + colocated[add_index].route.list_entry_name()
                log.append("Exit/Intersection: " + name + " -> " + label)
                return label

        # TODO: NY5@NY16/384&NY16@NY5/384&NY384@NY5/16
        # should become NY5/NY16/NY384
            
        # 3+ intersection with matching or partially matching labels
        # NY5@NY16/384&NY16@NY5/384&NY384@NY5/16
        # becomes NY5/NY16/NY384

        # or a more complex case:
        # US1@US21/176&US21@US1/378&US176@US1/378&US321@US1/378&US378@US21/176
        # becomes US1/US21/US176/US321/US378
        # approach: check if each label starts with some route number
        # in the list of colocated routes, and if so, create a label
        # slashing together all of the route names, and save any _
        # suffixes to put in and reduce the chance of conflicting names
        # and a second check to find matches when labels do not include
        # the abbrev field (which they often do not)
        if len(colocated) > 2:
            all_match = True
            suffixes = [""] * len(colocated)
            for check_index in range(len(colocated)):
                this_match = False
                for other_index in range(len(colocated)):
                    if other_index == check_index:
                        continue
                    if colocated[check_index].label.startswith(colocated[other_index].route.list_entry_name()):
                        # should check here for false matches, like
                        # NY50/67 would match startswith NY5
                        this_match = True
                        if '_' in colocated[check_index].label:
                            suffix = colocated[check_index].label[colocated[check_index].label.find('_'):]
                            if colocated[other_index].route.list_entry_name() + suffix == colocated[check_index].label:
                                suffixes[other_index] = suffix
                    if colocated[check_index].label.startswith(colocated[other_index].route.name_no_abbrev()):
                        this_match = True
                        if '_' in colocated[check_index].label:
                            suffix = colocated[check_index].label[colocated[check_index].label.find('_'):]
                            if colocated[other_index].route.name_no_abbrev() + suffix == colocated[check_index].label:
                                suffixes[other_index] = suffix
                if not this_match:
                    all_match = False
                    break
            if all_match:
                label = colocated[0].route.list_entry_name() + suffixes[0]
                for index in range(1,len(colocated)):
                    label += "/" + colocated[index].route.list_entry_name() + suffixes[index]
                log.append("3+ intersection: " + name + " -> " + label)
                return label

        # Exit number simplification: I-90@47B(94)&I-94@47B
        # becomes I-90/I-94@47B, with many other cases also matched
        # Still TODO: I-39@171C(90)&I-90@171C&US14@I-39/90
        # try each as a possible route@exit type situation and look
        # for matches
        for try_as_exit in range(len(colocated)):
            # see if all colocated points are potential matches
            # when considering the one at try_as_exit as a primary
            # exit number
            if not colocated[try_as_exit].label[0].isdigit():
                continue
            all_match = True
            # get the route number only version for one of the checks below
            route_number_only = colocated[try_as_exit].route.name_no_abbrev()
            for pos in range(len(route_number_only)):
                if route_number_only[pos].isdigit():
                    route_number_only = route_number_only[pos:]
                    break
            for try_as_match in range(len(colocated)):
                if try_as_exit == try_as_match:
                    continue
                this_match = False
                # check for any of the patterns that make sense as a match:
                # exact match, match without abbrev field, match with exit
                # number in parens, match concurrency exit number format
                # nn(rr), match with _ suffix (like _N), match with a slash
                # match with exit number only
                if (colocated[try_as_match].label == colocated[try_as_exit].route.list_entry_name()
                    or colocated[try_as_match].label == colocated[try_as_exit].route.name_no_abbrev()
                    or colocated[try_as_match].label == colocated[try_as_exit].route.list_entry_name() + "(" + colocated[try_as_exit].label + ")"
                    or colocated[try_as_match].label == colocated[try_as_exit].label + "(" + route_number_only + ")"
                    or colocated[try_as_match].label == colocated[try_as_exit].label + "(" + colocated[try_as_exit].route.name_no_abbrev() + ")"
                    or colocated[try_as_match].label.startswith(colocated[try_as_exit].route.name_no_abbrev() + "_")
                    or colocated[try_as_match].label.startswith(colocated[try_as_exit].route.name_no_abbrev() + "/")
                    or colocated[try_as_match].label == colocated[try_as_exit].label):
                    this_match = True
                if not this_match:
                    all_match = False

            if all_match:
                label = ""
                for pos in range(len(colocated)):
                    if pos == try_as_exit:
                        label += colocated[pos].route.list_entry_name() + "(" + colocated[pos].label + ")"
                    else:
                        label += colocated[pos].route.list_entry_name()
                    if pos < len(colocated) - 1:
                        label += "/"
                log.append("Exit number: " + name + " -> " + label)
                return label

        # TODO: I-20@76&I-77@16
        # should become I-20/I-77 or maybe I-20(76)/I-77(16)
        # not shorter, so maybe who cares about this one?

        # TODO: US83@FM1263_S&US380@FM1263
        # should probably end up as US83/US280@FM1263 or @FM1263_S

        # How about?
        # I-581@4&US220@I-581(4)&US460@I-581&US11AltRoa@I-581&US220AltRoa@US220_S&VA116@I-581(4)
        # INVESTIGATE: VA262@US11&US11@VA262&VA262@US11_S
        # should be 2 colocated, shows up as 3?

        # TODO: I-610@TX288&I-610@38&TX288@I-610
        # this is the overlap point of a loop

        # TODO: boundaries where order is reversed on colocated points
        # Vt4@FIN/NOR&E75@NOR/FIN&E75@NOR/FIN

        log.append("Keep failsafe: " + name)
        return name

    def simple_waypoint_name(self):
        """Failsafe name for a point, simply the string of route name @
        label, concatenated with & characters for colocated points."""
        if self.colocated is None:
            return self.route.list_entry_name() + "@" + self.label
        long_label = ""
        for w in self.colocated:
            if w.route.system.active_or_preview():
                if long_label != "":
                    long_label += "&"
                long_label += w.route.list_entry_name() + "@" + w.label
        return long_label

    def is_or_colocated_with_active_or_preview(self):
        if self.route.system.active_or_preview():
            return True
        if self.colocated is not None:
            for w in self.colocated:
                if w.route.system.active_or_preview():
                    return True
        return False

    def is_valid(self):
        return self.lat != 0.0 or self.lng != 0.0

    def hashpoint(self):
        # return a canonical waypoint for graph vertex hashtable lookup
        if self.colocated is None:
            return self
        return self.colocated[0]

class HighwaySegment:
    """This class represents one highway segment: the connection between two
    Waypoints connected by one or more routes"""

    def __init__(self,w1,w2,route):
        self.waypoint1 = w1
        self.waypoint2 = w2
        self.route = route
        self.concurrent = None
        self.clinched_by = []

    def __str__(self):
        return self.route.readable_name() + " " + self.waypoint1.label + " " + self.waypoint2.label

    def add_clinched_by(self,traveler):
        if traveler not in self.clinched_by:
            self.clinched_by.append(traveler)
            return True
        else:
            return False

    def csv_line(self,id):
        """return csv line to insert into a table"""
        return "'" + str(id) + "','" + str(self.waypoint1.point_num) + "','" + str(self.waypoint2.point_num) + "','" + self.route.root + "'"

    def length(self):
        """return segment length in miles"""
        return self.waypoint1.distance_to(self.waypoint2)

    def segment_name(self):
        """compute a segment name based on names of all
        concurrent routes, used for graph edge labels"""
        segment_name = ""
        if self.concurrent is None:
            if self.route.system.active_or_preview():
                segment_name += self.route.list_entry_name()
        else:
            for cs in self.concurrent:
                if cs.route.system.active_or_preview():
                    if segment_name != "":
                        segment_name += ","
                    segment_name += cs.route.list_entry_name()
        return segment_name

class Route:
    """This class encapsulates the contents of one .csv file line
    that represents a highway within a system and the corresponding
    information from the route's .wpt.

    The format of the .csv file for a highway system is a set of
    semicolon-separated lines, each of which has 8 fields:

    System;Region;Route;Banner;Abbrev;City;Route;AltRouteNames

    The first line names these fields, subsequent lines exist,
    one per highway, with values for each field.

    System: the name of the highway system this route belongs to,
    an instance of HighwaySystem

    Region: the project region or subdivision in which the
    route belongs.

    Route: the route name as would be specified in user lists

    Banner: the (optional) banner on the route such as 'Alt',
    'Bus', or 'Trk'.  Now allowed up to 6 characters

    Abbrev: (optional) for bannered routes or routes in multiple
    sections, the 3-letter abbrevation for the city or other place
    that is used to identify the segment.

    City: (optional) the full name to be displayed for the Abbrev
    above.

    Root: the name of the .wpt file that lists the waypoints of the
    route, without the .wpt extension.

    AltRouteNames: (optional) comma-separated list former or other
    alternate route names that might appear in user list files.
    """
    def __init__(self,line,system,el):
        """initialize object from a .csv file line, but do not
        yet read in waypoint file"""
        fields = line.split(";")
        if len(fields) != 8:
            el.add_error("Could not parse csv line: [" + line +
                         "], expected 8 fields, found " + str(len(fields)))
        self.system = system
        if system.systemname != fields[0]:
            el.add_error("System mismatch parsing line [" + line + "], expected " + system.systemname)
        self.region = fields[1]
        self.route = fields[2]
        self.banner = fields[3]
        self.abbrev = fields[4]
        self.city = fields[5].replace("'","''")
        self.root = fields[6]
        self.alt_route_names = fields[7].split(",")
        self.point_list = []
        self.labels_in_use = set()
        self.unused_alt_labels = set()
        self.segment_list = []
        self.mileage = 0.0
        self.rootOrder = -1  # order within connected route

    def __str__(self):
        """printable version of the object"""
        return self.root + " (" + str(len(self.point_list)) + " total points)"

    def read_wpt(self,all_waypoints,all_waypoints_lock,datacheckerrors,el,path="../../../HighwayData/hwy_data"):
        """read data into the Route's waypoint list from a .wpt file"""
        #print("read_wpt on " + str(self))
        self.point_list = []
        try:
            file = open(path+"/"+self.region + "/" + self.system.systemname+"/"+self.root+".wpt", "rt",encoding='utf-8')
        except OSError as e:
            el.add_error(str(e))
        else:
            lines = file.readlines()
            file.close()
            w = None
            for line in lines:
                line = line.strip()
                if len(line) > 0:
                    previous_point = w
                    w = Waypoint(line,self,datacheckerrors)
                    if w.is_valid() == False:
                        w = previous_point
                        continue
                    self.point_list.append(w)
                    # populate unused alt labels
                    for label in w.alt_labels:
                        self.unused_alt_labels.add(label.upper().strip("+"))
                    # look for colocated points
                    all_waypoints_lock.acquire()
                    other_w = all_waypoints.waypoint_at_same_point(w)
                    if other_w is not None:
                        # see if this is the first point colocated with other_w
                        if other_w.colocated is None:
                            other_w.colocated = [ other_w ]
                        other_w.colocated.append(w)
                        w.colocated = other_w.colocated

                    # look for near-miss points (before we add this one in)
                    #print("DEBUG: START search for nmps for waypoint " + str(w) + " in quadtree of size " + str(all_waypoints.size()))
                    #if not all_waypoints.is_valid():
                    #    sys.exit()
                    nmps = all_waypoints.near_miss_waypoints(w, 0.0005)
                    #print("DEBUG: for waypoint " + str(w) + " got " + str(len(nmps)) + " nmps: ", end="")
                    #for dbg_w in nmps:
                    #    print(str(dbg_w) + " ", end="")
                    #print()
                    if len(nmps) > 0:
                        if w.near_miss_points is None:
                            w.near_miss_points = nmps
                        else:
                            w.near_miss_points.extend(nmps)
    
                        for other_w in nmps:
                            if other_w.near_miss_points is None:
                                other_w.near_miss_points = [ w ]
                            else:
                                other_w.near_miss_points.append(w)
    
                    all_waypoints.insert(w)
                    all_waypoints_lock.release()
                    # add HighwaySegment, if not first point
                    if previous_point is not None:
                        self.segment_list.append(HighwaySegment(previous_point, w, self))

    def print_route(self):
        for point in self.point_list:
            print(str(point))

    def find_segment_by_waypoints(self,w1,w2):
        for s in self.segment_list:
            if s.waypoint1 is w1 and s.waypoint2 is w2 or s.waypoint1 is w2 and s.waypoint2 is w1:
                return s
        return None

    def csv_line(self):
        """return csv line to insert into a table"""
        # note: alt_route_names does not need to be in the db since
        # list preprocessing uses alt or canonical and no longer cares
        return "'" + self.system.systemname + "','" + self.region + "','" + self.route + "','" + self.banner + "','" + self.abbrev + "','" + self.city + "','" + self.root + "','" + str(self.mileage) + "','" + str(self.rootOrder) + "'";

    def readable_name(self):
        """return a string for a human-readable route name"""
        return self.region+ " " + self.route + self.banner + self.abbrev

    def list_entry_name(self):
        """return a string for a human-readable route name in the
        format expected in traveler list files"""
        return self.route + self.banner + self.abbrev

    def name_no_abbrev(self):
        """return a string for a human-readable route name in the
        format that might be encountered for intersecting route
        labels, where the abbrev field is often omitted"""
        return self.route + self.banner

    def clinched_by_traveler(self,t):
        miles = 0.0
        for s in self.segment_list:
            if t in s.clinched_by:
                miles += s.length()
        return miles

class ConnectedRoute:
    """This class encapsulates a single 'connected route' as given
    by a single line of a _con.csv file
    """

    def __init__(self,line,system,el):
        """initialize the object from the _con.csv line given"""
        fields = line.split(";")
        if len(fields) != 5:
            el.add_error("Could not parse _con.csv line: [" + line +
                         "] expected 5 fields, found " + str(len(fields)))
        self.system = system
        if system.systemname != fields[0]:
            el.add_error("System mismatch parsing line [" + line + "], expected " + system.systemname)
        self.route = fields[1]
        self.banner = fields[2]
        self.groupname = fields[3]
        # fields[4] is the list of roots, which will become a python list
        # of Route objects already in the system
        self.roots = []
        roots = fields[4].split(",")
        rootOrder = 0
        for root in roots:
            route = None
            for check_route in system.route_list:
                if check_route.root == root:
                    route = check_route
                    break
            if route is None:
                el.add_error("Could not find Route matching root " + root +
                             " in system " + system.systemname + '.')
            else:
                self.roots.append(route)
                # save order of route in connected route
                route.rootOrder = rootOrder
            rootOrder += 1
        if len(self.roots) < 1:
            el.add_error("No roots in _con.csv line [" + line + "]")
        # will be computed for routes in active & preview systems later
        self.mileage = 0.0

    def csv_line(self):
        """return csv line to insert into a table"""
        return "'" + self.system.systemname + "','" + self.route + "','" + self.banner + "','" + self.groupname.replace("'","''") + "','" + self.roots[0].root + "','" + str(self.mileage) + "'";

    def readable_name(self):
        """return a string for a human-readable connected route name"""
        ans = self.route + self.banner
        if self.groupname != "":
            ans += " (" +  self.groupname + ")"
        return ans

class HighwaySystem:
    """This class encapsulates the contents of one .csv file
    that represents the collection of highways within a system.

    See Route for information about the fields of a .csv file

    With the implementation of three tiers of systems (active,
    preview, devel), added parameter and field here, to be stored in
    DB

    After construction and when all Route entries are made, a _con.csv
    file is read that defines the connected routes in the system.
    In most cases, the connected route is just a single Route, but when
    a designation within the same system crosses region boundaries,
    a connected route defines the entirety of the route.
    """
    def __init__(self,systemname,country,fullname,color,tier,level,el,
                 path="../../../HighwayData/hwy_data/_systems"):
        self.route_list = []
        self.con_route_list = []
        self.systemname = systemname
        self.country = country
        self.fullname = fullname
        self.color = color
        self.tier = tier
        self.level = level
        self.mileage_by_region = dict()
        try:
            file = open(path+"/"+systemname+".csv","rt",encoding='utf-8')
        except OSError as e:
            el.add_error(str(e))
        else:
            lines = file.readlines()
            file.close()
            # ignore the first line of field names
            lines.pop(0)
            for line in lines:
                self.route_list.append(Route(line.rstrip('\n'),self,el))
        try:
            file = open(path+"/"+systemname+"_con.csv","rt",encoding='utf-8')
        except OSError as e:
            el.add_error(str(e))
        else:
            lines = file.readlines()
            file.close()
            # again, ignore first line with field names
            lines.pop(0)
            for line in lines:
                self.con_route_list.append(ConnectedRoute(line.rstrip('\n'),
                                                          self,el))

    """Return whether this is an active system"""
    def active(self):
        return self.level == "active"

    """Return whether this is a preview system"""
    def preview(self):
        return self.level == "preview"

    """Return whether this is an active or preview system"""
    def active_or_preview(self):
        return self.level == "active" or self.level == "preview"

    """Return whether this is a development system"""
    def devel(self):
        return self.level == "devel"

    """String representation"""
    def __str__(self):
        return self.systemname

class TravelerList:
    """This class encapsulates the contents of one .list file
    that represents the travels of one individual user.

    A list file consists of lines of 4 values:
    region route_name start_waypoint end_waypoint

    which indicates that the user has traveled the highway names
    route_name in the given region between the waypoints named
    start_waypoint end_waypoint
    """

    def __init__(self,travelername,route_hash,path="../../../UserData/list_files"):
        self.list_entries = []
        self.clinched_segments = set()
        self.traveler_name = travelername[:-5]
        with open(path+"/"+travelername,"rt", encoding='UTF-8') as file:
            lines = file.readlines()
        file.close()

        self.log_entries = []

        for line in lines:
            line = line.strip().rstrip('\x00')
            # ignore empty or "comment" lines
            if len(line) == 0 or line.startswith("#"):
                continue
            fields = re.split(' +',line)
            if len(fields) != 4:
                # OK if 5th field exists and starts with #
                if len(fields) < 5 or not fields[4].startswith("#"):
                    self.log_entries.append("Incorrect format line: " + line)
                    continue

            # find the root that matches in some system and when we do, match labels
            route_entry = fields[1].lower()
            lookup = fields[0].lower() + ' ' + route_entry
            if lookup not in route_hash:
                self.log_entries.append("Unknown region/highway combo in line: " + line)
            else:
                r = route_hash[lookup]
                for a in r.alt_route_names:
                    if route_entry == a.lower():
                        self.log_entries.append("Note: deprecated route name " + fields[1] + " -> canonical name " + r.list_entry_name() + " in line " + line)
                        break

                if r.system.devel():
                    self.log_entries.append("Ignoring line matching highway in system in development: " + line)
                    continue
                # r is a route match, r.root is our root, and we need to find
                # canonical waypoint labels, ignoring case and leading
                # "+" or "*" when matching
                canonical_waypoints = []
                canonical_waypoint_indices = []
                checking_index = 0;
                for w in r.point_list:
                    lower_label = w.label.lower().strip("+*")
                    list_label_1 = fields[2].lower().strip("+*")
                    list_label_2 = fields[3].lower().strip("+*")
                    if list_label_1 == lower_label or list_label_2 == lower_label:
                        canonical_waypoints.append(w)
                        canonical_waypoint_indices.append(checking_index)
                        r.labels_in_use.add(lower_label.upper())
                    else:
                        for alt in w.alt_labels:
                            lower_label = alt.lower().strip("+")
                            if list_label_1 == lower_label or list_label_2 == lower_label:
                                canonical_waypoints.append(w)
                                canonical_waypoint_indices.append(checking_index)
                                r.labels_in_use.add(lower_label.upper())
                                # if we have not yet used this alt label, remove it from the unused list
                                if lower_label.upper() in r.unused_alt_labels:
                                    r.unused_alt_labels.remove(lower_label.upper())
                                            
                    checking_index += 1
                if len(canonical_waypoints) != 2:
                    self.log_entries.append("Waypoint label(s) not found in line: " + line)
                else:
                    self.list_entries.append(ClinchedSegmentEntry(line, r.root, \
                                                                  canonical_waypoints[0].label, \
                                                                  canonical_waypoints[1].label))
                    # find the segments we just matched and store this traveler with the
                    # segments and the segments with the traveler (might not need both
                    # ultimately)
                    #start = r.point_list.index(canonical_waypoints[0])
                    #end = r.point_list.index(canonical_waypoints[1])
                    start = canonical_waypoint_indices[0]
                    end = canonical_waypoint_indices[1]
                    for wp_pos in range(start,end):
                        hs = r.segment_list[wp_pos] #r.get_segment(r.point_list[wp_pos], r.point_list[wp_pos+1])
                        hs.add_clinched_by(self)
                        if hs not in self.clinched_segments:
                            self.clinched_segments.add(hs)

        self.log_entries.append("Processed " + str(len(self.list_entries)) + \
                                    " good lines marking " +str(len(self.clinched_segments)) + \
                                    " segments traveled.")
        # additional setup for later stats processing
        # a place to track this user's total mileage per region,
        # but only active+preview and active only (since devel
        # systems are not clinchable)
        self.active_preview_mileage_by_region = dict()
        self.active_only_mileage_by_region = dict()
        # a place for this user's total mileage per system, again by region
        # this will be a dictionary of dictionaries, keys of the top level
        # are system names (e.g., 'usai') and values are dictionaries whose
        # keys are region names and values are total mileage in that
        # system in that region
        self.system_region_mileages = dict()

    def write_log(self,path="."):
        logfile = open(path+"/"+self.traveler_name+".log","wt",encoding='UTF-8')
        logfile.write("Log file created at: " + str(datetime.datetime.now()) + "\n")
        for line in self.log_entries:
            logfile.write(line + "\n")
        logfile.close()

class ClinchedSegmentEntry:
    """This class encapsulates one line of a traveler's list file

    raw_line is the actual line from the list file for error reporting

    root is the root name of the route clinched

    canonical_start and canonical_end are waypoint labels, which must be
    in the same order as they appear in the route decription file, and
    must be primary labels
    """

    def __init__(self,line,root,canonical_start,canonical_end):
        self.raw_line = line
        self.root = root
        self.canonical_start = canonical_start
        self.canonical_end = canonical_end

class DatacheckEntry:
    """This class encapsulates a datacheck log entry

    route is a reference to the route with a datacheck error

    labels is a list of labels that are related to the error (such
    as the endpoints of a too-long segment or the three points that
    form a sharp angle)

    code is the error code string, one of SHARP_ANGLE, BAD_ANGLE,
    DUPLICATE_LABEL, DUPLICATE_COORDS, LABEL_SELFREF,
    LABEL_INVALID_CHAR, LONG_SEGMENT, MALFORMED_URL,
    LABEL_UNDERSCORES, VISIBLE_DISTANCE, LABEL_PARENS, LACKS_GENERIC,
    BUS_WITH_I, NONTERMINAL_UNDERSCORE,
    LONG_UNDERSCORE, LABEL_SLASHES, US_BANNER, VISIBLE_HIDDEN_COLOC,
    HIDDEN_JUNCTION, LABEL_LOOKS_HIDDEN, HIDDEN_TERMINUS,
    OUT_OF_BOUNDS

    info is additional information, at this time either a distance (in
    miles) for a long segment error, an angle (in degrees) for a sharp
    angle error, or a coordinate pair for duplicate coordinates, other
    route/label for point pair errors

    fp is a boolean indicating whether this has been reported as a
    false positive (would be set to true later)

    """

    def __init__(self,route,labels,code,info=""):
         self.route = route
         self.labels = labels
         self.code = code
         self.info = info
         self.fp = False

    def match_except_info(self,fpentry):
        """Check if the fpentry from the csv file matches in all fields
        except the info field"""
        # quick and easy checks first
        if self.route.root != fpentry[0] or self.code != fpentry[4]:
            return False
        # now label matches
        if len(self.labels) > 0 and self.labels[0] != fpentry[1]:
            return False
        if len(self.labels) > 1 and self.labels[1] != fpentry[2]:
            return False
        if len(self.labels) > 2 and self.labels[2] != fpentry[3]:
            return False
        return True

    def __str__(self):
        entry = str(self.route.root)+";"
        if len(self.labels) == 0:
            entry += ";;;"
        elif len(self.labels) == 1:
            entry += self.labels[0]+";;;"
        elif len(self.labels) == 2:
            entry += self.labels[0]+";"+self.labels[1]+";;"
        else:
            entry += self.labels[0]+";"+self.labels[1]+";"+self.labels[2]+";"
        entry += self.code+";"+self.info
        return entry

class HighwayGraphVertexInfo:
    """This class encapsulates information needed for a highway graph
    vertex.
    """

    def __init__(self,wpt,unique_name,datacheckerrors):
        self.lat = wpt.lat
        self.lng = wpt.lng
        self.unique_name = unique_name
        # will consider hidden iff all colocated waypoints are hidden
        self.is_hidden = True
        # note: if saving the first waypoint, no longer need
        # lat & lng and can replace with methods
        self.first_waypoint = wpt
        self.regions = set()
        self.systems = set()
        self.incident_edges = []
        self.incident_collapsed_edges = []
        if wpt.colocated is None:
            if not wpt.is_hidden:
                self.is_hidden = False
            self.regions.add(wpt.route.region)
            self.systems.add(wpt.route.system)
            return
        for w in wpt.colocated:
            if not w.is_hidden:
                self.is_hidden = False
            self.regions.add(w.route.region)
            self.systems.add(w.route.system)
        # VISIBLE_HIDDEN_COLOC datacheck
        for p in range(1, len(wpt.colocated)):
            if wpt.colocated[p].is_hidden != wpt.colocated[0].is_hidden:
                # determine which route, label, and info to use for this entry asciibetically
                vis_list = []
                hid_list = []
                for w in wpt.colocated:
                    if w.is_hidden:
                        hid_list.append(w)
                    else:
                        vis_list.append(w)
                datacheckerrors.append(DatacheckEntry(vis_list[0].route,[vis_list[0].label],"VISIBLE_HIDDEN_COLOC",
                                                      hid_list[0].route.root+"@"+hid_list[0].label))
                break;

    # printable string
    def __str__(self):
        return self.unique_name

class HighwayGraphEdgeInfo:
    """This class encapsulates information needed for a 'standard'
    highway graph edge.
    """

    def __init__(self,s,graph):
        # temp debug
        self.written = False
        self.segment_name = s.segment_name()
        self.vertex1 = graph.vertices[s.waypoint1.hashpoint()]
        self.vertex2 = graph.vertices[s.waypoint2.hashpoint()]
        # assumption: each edge/segment lives within a unique region
        self.region = s.route.region
        # a list of route name/system pairs
        self.route_names_and_systems = []
        if s.concurrent is None:
            self.route_names_and_systems.append((s.route.list_entry_name(), s.route.system))
        else:
            for cs in s.concurrent:
                if cs.route.system.devel():
                    continue
                self.route_names_and_systems.append((cs.route.list_entry_name(), cs.route.system))

        # checks for the very unusual cases where an edge ends up
        # in the system as itself and its "reverse"
        duplicate = False
        for e in self.vertex1.incident_edges:
            if e.vertex1 == self.vertex2 and e.vertex2 == self.vertex1:
                duplicate = True

        for e in self.vertex2.incident_edges:
            if e.vertex1 == self.vertex2 and e.vertex2 == self.vertex1:
                duplicate = True

        if not duplicate:
            self.vertex1.incident_edges.append(self)
            self.vertex2.incident_edges.append(self)
        else:
            # flag as invalid/duplicate in order to bypass
            # building a HighwayGraphCollapsedEdgeInfo
            self.vertex1 = None

    # compute an edge label, optionally resticted by systems
    def label(self,systems=None):
        the_label = ""
        for (name, system) in self.route_names_and_systems:
            if systems is None or system in systems:
                if the_label == "":
                    the_label = name
                else:
                    the_label += ","+name

        return the_label

    # printable string for this edge
    def __str__(self):
        return "HighwayGraphEdgeInfo: " + self.segment_name + " from " + str(self.vertex1) + " to " + str(self.vertex2)

class HighwayGraphCollapsedEdgeInfo:
    """This class encapsulates information needed for a highway graph
    edge that can incorporate intermediate points.
    """

    def __init__(self,HGEdge=None,vertex_info=None):
        if HGEdge is None and vertex_info is None:
            print("ERROR: improper use of HighwayGraphCollapsedEdgeInfo constructor\n")
            return

        # a few items we can do for either construction type
        self.written = False

        # intermediate points, if more than 1, will go from vertex1 to
        # vertex2
        self.intermediate_points = []

        # initial construction is based on a HighwayGraphEdgeInfo
        if HGEdge is not None:
            self.segment_name = HGEdge.segment_name
            self.vertex1 = HGEdge.vertex1
            self.vertex2 = HGEdge.vertex2
            # assumption: each edge/segment lives within a unique region
            # and a 'multi-edge' would not be able to span regions as there
            # would be a required visible waypoint at the border
            self.region = HGEdge.region
            # a list of route name/system pairs
            self.route_names_and_systems = HGEdge.route_names_and_systems
            self.vertex1.incident_collapsed_edges.append(self)
            self.vertex2.incident_collapsed_edges.append(self)

        # build by collapsing two existing edges around a common
        # hidden vertex waypoint, whose information is given in
        # vertex_info
        if vertex_info is not None:
            # we know there are exactly 2 incident edges, as we
            # checked for that, and we will replace these two
            # with the single edge we are constructing here
            edge1 = vertex_info.incident_collapsed_edges[0]
            edge2 = vertex_info.incident_collapsed_edges[1]
            # segment names should match as routes should not start or end
            # nor should concurrencies begin or end at a hidden point
            if edge1.segment_name != edge2.segment_name:
                print("ERROR: segment name mismatch in HighwayGraphCollapsedEdgeInfo: edge1 named " + edge1.segment_name + " edge2 named " + edge2.segment_name + "\n")
            self.segment_name = edge1.segment_name
            #print("\nDEBUG: collapsing edges along " + self.segment_name + " at vertex " + str(vertex_info) + ", edge1 is " + str(edge1) + " and edge2 is " + str(edge2))
            # region and route names/systems should also match, but not
            # doing that sanity check here, as the above check should take
            # care of that
            self.region = edge1.region
            self.route_names_and_systems = edge1.route_names_and_systems

            # figure out and remember which endpoints are not the
            # vertex we are collapsing and set them as our new
            # endpoints, and at the same time, build up our list of
            # intermediate vertices
            self.intermediate_points = edge1.intermediate_points.copy()
            #print("DEBUG: copied edge1 intermediates" + self.intermediate_point_string())

            if edge1.vertex1 == vertex_info:
                #print("DEBUG: self.vertex1 getting edge1.vertex2: " + str(edge1.vertex2) + " and reversing edge1 intermediates")
                self.vertex1 = edge1.vertex2
                self.intermediate_points.reverse()
            else:
                #print("DEBUG: self.vertex1 getting edge1.vertex1: " + str(edge1.vertex1))
                self.vertex1 = edge1.vertex1

            #print("DEBUG: appending to intermediates: " + str(vertex_info))
            self.intermediate_points.append(vertex_info)

            toappend = edge2.intermediate_points.copy()
            #print("DEBUG: copied edge2 intermediates" + edge2.intermediate_point_string())
            if edge2.vertex1 == vertex_info:
                #print("DEBUG: self.vertex2 getting edge2.vertex2: " + str(edge2.vertex2))
                self.vertex2 = edge2.vertex2
            else:
                #print("DEBUG: self.vertex2 getting edge2.vertex1: " + str(edge2.vertex1) + " and reversing edge2 intermediates")
                self.vertex2 = edge2.vertex1
                toappend.reverse()

            self.intermediate_points.extend(toappend)

            #print("DEBUG: intermediates complete: from " + str(self.vertex1) + " via " + self.intermediate_point_string() + " to " + str(self.vertex2))

            # replace edge references at our endpoints with ourself
            removed = 0
            if edge1 in self.vertex1.incident_collapsed_edges:
                self.vertex1.incident_collapsed_edges.remove(edge1)
                removed += 1
            if edge1 in self.vertex2.incident_collapsed_edges:
                self.vertex2.incident_collapsed_edges.remove(edge1)
                removed += 1
            if removed != 1:
                print("ERROR: edge1 " + str(edge1) + " removed from " + str(removed) + " adjacency lists instead of 1.")
            removed = 0
            if edge2 in self.vertex1.incident_collapsed_edges:
                self.vertex1.incident_collapsed_edges.remove(edge2)
                removed += 1
            if edge2 in self.vertex2.incident_collapsed_edges:
                self.vertex2.incident_collapsed_edges.remove(edge2)
                removed += 1
            if removed != 1:
                print("ERROR: edge2 " + str(edge2) + " removed from " + str(removed) + " adjacency lists instead of 1.")
            self.vertex1.incident_collapsed_edges.append(self)
            self.vertex2.incident_collapsed_edges.append(self)


    # compute an edge label, optionally resticted by systems
    def label(self,systems=None):
        the_label = ""
        for (name, system) in self.route_names_and_systems:
            if systems is None or system in systems:
                if the_label == "":
                    the_label = name
                else:
                    the_label += ","+name

        return the_label

    # printable string for this edge
    def __str__(self):
        return "HighwayGraphCollapsedEdgeInfo: " + self.segment_name + " from " + str(self.vertex1) + " to " + str(self.vertex2) + " via " + str(len(self.intermediate_points)) + " points"

    # line appropriate for a tmg collapsed edge file
    def collapsed_tmg_line(self, systems=None):
        line = str(self.vertex1.vis_vertex_num) + " " + str(self.vertex2.vis_vertex_num) + " " + self.label(systems)
        for intermediate in self.intermediate_points:
            line += " " + str(intermediate.lat) + " " + str(intermediate.lng)
        return line

    # line appropriate for a tmg collapsed edge file, with debug info
    def debug_tmg_line(self, systems=None):
        line = str(self.vertex1.vertex_num) + " [" + self.vertex1.unique_name + "] " + str(self.vertex2.vertex_num) + " [" + self.vertex2.unique_name + "] " + self.label(systems)
        for intermediate in self.intermediate_points:
            line += " [" + intermediate.unique_name + "] " + str(intermediate.lat) + " " + str(intermediate.lng)
        return line

    # return the intermediate points as a string
    def intermediate_point_string(self):
        if len(self.intermediate_points) == 0:
            return " None"

        line = ""
        for intermediate in self.intermediate_points:
            line += " [" + intermediate.unique_name + "] " + str(intermediate.lat) + " " + str(intermediate.lng)
        return line

class PlaceRadius:
    """This class encapsulates a place name, file base name, latitude,
    longitude, and radius (in miles) to define the area to which our
    place-based graphs are restricted.
    """

    def __init__(self, place, base, lat, lng, r):
        self.place = place
        self.base = base
        self.lat = float(lat)
        self.lng = float(lng)
        self.r = int(r)

    def contains_vertex_info(self, vinfo):
        """return whether vinfo's coordinates are within this area"""
        # convert to radians to compte distance
        rlat1 = math.radians(self.lat)
        rlng1 = math.radians(self.lng)
        rlat2 = math.radians(vinfo.lat)
        rlng2 = math.radians(vinfo.lng)

        ans = math.acos(math.cos(rlat1)*math.cos(rlng1)*math.cos(rlat2)*math.cos(rlng2) +\
                        math.cos(rlat1)*math.sin(rlng1)*math.cos(rlat2)*math.sin(rlng2) +\
                        math.sin(rlat1)*math.sin(rlat2)) * 3963.1 # EARTH_RADIUS;
        return ans <= self.r

    def contains_waypoint(self, w):
        """return whether w is within this area"""
        # convert to radians to compte distance
        rlat1 = math.radians(self.lat)
        rlng1 = math.radians(self.lng)
        rlat2 = math.radians(w.lat)
        rlng2 = math.radians(w.lng)

        ans = math.acos(math.cos(rlat1)*math.cos(rlng1)*math.cos(rlat2)*math.cos(rlng2) +\
                        math.cos(rlat1)*math.sin(rlng1)*math.cos(rlat2)*math.sin(rlng2) +\
                        math.sin(rlat1)*math.sin(rlat2)) * 3963.1 # EARTH_RADIUS;
        return ans <= self.r

    def contains_edge(self, e):
        """return whether both endpoints of edge e are within this area"""
        return (self.contains_waypoint(e.vertex1) and
                self.contains_waypoint(e.vertex2))
        
class HighwayGraph:
    """This class implements the capability to create graph
    data structures representing the highway data.

    On construction, build a set of unique vertex names
    and determine edges, at most one per concurrent segment.
    Create two sets of edges - one for the full graph
    and one for the graph with hidden waypoints compressed into
    multi-point edges.
    """

    def __init__(self, all_waypoints, highway_systems, datacheckerrors):
        # first, find unique waypoints and create vertex labels
        vertex_names = set()
        self.vertices = {}
        all_waypoint_list = all_waypoints.point_list()

        # add a unique name field to each waypoint, initialized to
        # None, which should get filled in later for any waypoint that
        # is or shares a location with any waypoint in an active or
        # preview system
        for w in all_waypoint_list:
            w.unique_name = None

        # to track the waypoint name compressions, add log entries
        # to this list
        self.waypoint_naming_log = []

        # loop for each Waypoint, create a unique name and vertex
        # unless it's a point not in or colocated with any active
        # or preview system, or is colocated and not at the front
        # of its colocation list
        for w in all_waypoint_list:
            # skip if this point is occupied by only waypoints in
            # devel systems
            if not w.is_or_colocated_with_active_or_preview():
                continue

            # skip if colocated and not at front of list
            if w.colocated is not None and w != w.colocated[0]:
                continue

            # come up with a unique name that brings in its meaning

            # start with the canonical name
            point_name = w.canonical_waypoint_name(self.waypoint_naming_log)

            # if that's taken, append the region code
            if point_name in vertex_names:
                point_name += "|" + w.route.region
                self.waypoint_naming_log.append("Appended region: " + point_name)

            # if that's taken, see if the simple name
            # is available
            if point_name in vertex_names:
                simple_name = w.simple_waypoint_name()
                if simple_name not in vertex_names:
                    self.waypoint_naming_log.append("Revert to simple: " + simple_name + " from (taken) " + point_name)
                    point_name = simple_name

            # if we have not yet succeeded, add !'s until we do
            while point_name in vertex_names:
                point_name += "!"
                self.waypoint_naming_log.append("Appended !: " + point_name)

            # we're good; now add point_name to the set and construct a vertex
            vertex_names.add(point_name)
            if w.colocated is None:
                self.vertices[w] = HighwayGraphVertexInfo(w, point_name, datacheckerrors)
            else:
                self.vertices[w] = HighwayGraphVertexInfo(w.colocated[0], point_name, datacheckerrors)

        # now that vertices are in place with names, set of unique names is no longer needed
        vertex_names = None

        # add edges, which end up in two separate vertex adjacency lists,
        for h in highway_systems:
            if h.devel():
                continue
            for r in h.route_list:
                for s in r.segment_list:
                    if s.concurrent is None or s == s.concurrent[0]:
                        # first one copy for the full simple graph
                        e = HighwayGraphEdgeInfo(s, self)
                        # and again for a graph where hidden waypoints
                        # are merged into the edge structures
                        if e.vertex1 is not None:
                            HighwayGraphCollapsedEdgeInfo(HGEdge=e)
                        else:
                            e = None

        print("Full graph has " + str(len(self.vertices)) +
              " vertices, " + str(self.edge_count()) + " edges.")

        # compress edges adjacent to hidden vertices
        for label, vinfo in self.vertices.items():
            if vinfo.is_hidden:
                if len(vinfo.incident_collapsed_edges) < 2:
                    # these cases are flagged as HIDDEN_TERMINUS
                    vinfo.is_hidden = False
                    continue
                if len(vinfo.incident_collapsed_edges) > 2:
                    datacheckerrors.append(DatacheckEntry(vinfo.first_waypoint.colocated[0].route,
                                           [vinfo.first_waypoint.colocated[0].label],
                                           "HIDDEN_JUNCTION",str(len(vinfo.incident_collapsed_edges))))
                    vinfo.is_hidden = False
                    continue
                # construct from vertex_info this time
                HighwayGraphCollapsedEdgeInfo(vertex_info=vinfo)

        # print summary info
        print("Edge compressed graph has " + str(self.num_visible_vertices()) +
              " vertices, " + str(self.collapsed_edge_count()) + " edges.")

    def num_visible_vertices(self):
        count = 0
        for v in self.vertices.values():
            if not v.is_hidden:
                count += 1
        return count

    def edge_count(self):
        edges = 0
        for v in self.vertices.values():
            edges += len(v.incident_edges)
        return edges//2

    def collapsed_edge_count(self):
        edges = 0
        for v in self.vertices.values():
            if not v.is_hidden:
                edges += len(v.incident_collapsed_edges)
        return edges//2

    def matching_vertices(self, regions, systems, placeradius):
        # return a list of vertices from the graph, optionally
        # restricted by region or system or placeradius area
        vis = 0
        vertex_list = []
        for vinfo in self.vertices.values():
            if placeradius is not None and not placeradius.contains_vertex_info(vinfo):
                continue
            region_match = regions is None
            if not region_match:
                for r in regions:
                    if r in vinfo.regions:
                        region_match = True
                        break
            if not region_match:
                continue
            system_match = systems is None
            if not system_match:
                for s in systems:
                    if s in vinfo.systems:
                        system_match = True
                        break
            if not system_match:
                continue
            if not vinfo.is_hidden:
                vis += 1
            vertex_list.append(vinfo)
        return (vertex_list, vis)

    def matching_edges(self, mv, regions=None, systems=None, placeradius=None):
        # return a set of edges from the graph, optionally
        # restricted by region or system or placeradius area
        edge_set = set()
        for v in mv:
            for e in v.incident_edges:
                if placeradius is None or placeradius.contains_edge(e):
                    if regions is None or e.region in regions:
                        system_match = systems is None
                        if not system_match:
                            for (r, s) in e.route_names_and_systems:
                                if s in systems:
                                    system_match = True
                        if system_match:
                            edge_set.add(e)
        return edge_set

    def matching_collapsed_edges(self, mv, regions=None, systems=None,
                                 placeradius=None):
        # return a set of edges from the graph edges for the collapsed
        # edge format, optionally restricted by region or system or
        # placeradius area
        edge_set = set()
        for v in mv:
            if v.is_hidden:
                continue
            for e in v.incident_collapsed_edges:
                if placeradius is None or placeradius.contains_edge(e):
                    if regions is None or e.region in regions:
                        system_match = systems is None
                        if not system_match:
                            for (r, s) in e.route_names_and_systems:
                                if s in systems:
                                    system_match = True
                        if system_match:
                            edge_set.add(e)
        return edge_set

    # write the entire set of highway data a format very similar to
    # the original .gra format.  The first line is a header specifying
    # the format and version number, the second line specifying the
    # number of waypoints, w, and the number of connections, c, then w
    # lines describing waypoints (label, latitude, longitude), then c
    # lines describing connections (endpoint 1 number, endpoint 2
    # number, route label)
    #
    # returns tuple of number of vertices and number of edges written
    #
    def write_master_tmg_simple(self,filename):
        tmgfile = open(filename, 'w')
        tmgfile.write("TMG 1.0 simple\n")
        tmgfile.write(str(len(self.vertices)) + ' ' + str(self.edge_count()) + '\n')
        # number waypoint entries as we go to support original .gra
        # format output
        vertex_num = 0
        for vinfo in self.vertices.values():
            tmgfile.write(vinfo.unique_name + ' ' + str(vinfo.lat) + ' ' + str(vinfo.lng) + '\n')
            vinfo.vertex_num = vertex_num
            vertex_num += 1

        # sanity check
        if len(self.vertices) != vertex_num:
            print("ERROR: computed " + str(len(self.vertices)) + " waypoints but wrote " + str(vertex_num))

        # now edges, only print if not already printed
        edge = 0
        for v in self.vertices.values():
            for e in v.incident_edges:
                if not e.written:
                    e.written = True
                    tmgfile.write(str(e.vertex1.vertex_num) + ' ' + str(e.vertex2.vertex_num) + ' ' + e.label() + '\n')
                    edge += 1

        # sanity checks
        for v in self.vertices.values():
            for e in v.incident_edges:
                if not e.written:
                    print("ERROR: never wrote edge " + str(e.vertex1.vertex_num) + ' ' + str(e.vertex2.vertex_num) + ' ' + e.label() + '\n')
        if self.edge_count() != edge:
            print("ERROR: computed " + str(self.edge_count()) + " edges but wrote " + str(edge) + "\n")

        tmgfile.close()
        return (len(self.vertices), self.edge_count())

    # write the entire set of data in the tmg collapsed edge format
    def write_master_tmg_collapsed(self, filename):
        tmgfile = open(filename, 'w')
        tmgfile.write("TMG 1.0 collapsed\n")
        print("(" + str(self.num_visible_vertices()) + "," +
              str(self.collapsed_edge_count()) + ") ", end="", flush=True)
        tmgfile.write(str(self.num_visible_vertices()) + " " +
                      str(self.collapsed_edge_count()) + "\n")

        # write visible vertices
        vis_vertex_num = 0
        for vinfo in self.vertices.values():
            if not vinfo.is_hidden:
                vinfo.vis_vertex_num = vis_vertex_num
                tmgfile.write(vinfo.unique_name + ' ' + str(vinfo.lat) + ' ' + str(vinfo.lng) + '\n')
                vis_vertex_num += 1

        # write collapsed edges
        edge = 0
        for v in self.vertices.values():
            if not v.is_hidden:
                for e in v.incident_collapsed_edges:
                    if not e.written:
                        e.written = True
                        tmgfile.write(e.collapsed_tmg_line() + '\n')
                        edge += 1

        # sanity check on edges written
        if self.collapsed_edge_count() != edge:
            print("ERROR: computed " + str(self.collapsed_edge_count()) + " collapsed edges, but wrote " + str(edge) + "\n")

        tmgfile.close()
        return (self.num_visible_vertices(), self.collapsed_edge_count())

    # write a subset of the data,
    # in both simple and collapsed formats,
    # restricted by regions in the list if given,
    # by system in the list if given,
    # or to within a given area if placeradius is given
    def write_subgraphs_tmg(self, graph_list, path, root, descr, category, regions, systems, placeradius):
        visible = 0
        simplefile = open(path+root+"-simple.tmg","w",encoding='utf-8')
        collapfile = open(path+root+".tmg","w",encoding='utf-8')
        (mv, visible) = self.matching_vertices(regions, systems, placeradius)
        mse = self.matching_edges(mv, regions, systems, placeradius)
        mce = self.matching_collapsed_edges(mv, regions, systems, placeradius)
        print('(' + str(len(mv)) + ',' + str(len(mse)) + ") ", end="", flush=True)
        print('(' + str(visible) + ',' + str(len(mce)) + ") ", end="", flush=True)
        simplefile.write("TMG 1.0 simple\n")
        collapfile.write("TMG 1.0 collapsed\n")
        simplefile.write(str(len(mv)) + ' ' + str(len(mse)) + '\n')
        collapfile.write(str(visible) + ' ' + str(len(mce)) + '\n')

        # write vertices
        sv = 0
        cv = 0
        for v in mv:
            # all vertices, for simple graph
            simplefile.write(v.unique_name + ' ' + str(v.lat) + ' ' + str(v.lng) + '\n')
            v.vertex_num = sv
            sv += 1
            # visible vertices, for collapsed graph
            if not v.is_hidden:
                collapfile.write(v.unique_name + ' ' + str(v.lat) + ' ' + str(v.lng) + '\n')
                v.vis_vertex_num = cv
                cv += 1
        # write edges
        for e in mse:
            simplefile.write(str(e.vertex1.vertex_num) + ' ' + str(e.vertex2.vertex_num) + ' ' + e.label(systems) + '\n')
        for e in mce:
            collapfile.write(e.collapsed_tmg_line(systems) + '\n')
        simplefile.close()
        collapfile.close()

        graph_list.append(GraphListEntry(root+"-simple.tmg", descr, len(mv), len(mse), "simple", category))
        graph_list.append(GraphListEntry(root   +    ".tmg", descr, visible, len(mce), "collapsed", category))

def format_clinched_mi(clinched,total):
    """return a nicely-formatted string for a given number of miles
    clinched and total miles, including percentage"""
    percentage = "-.-%"
    if total != 0.0:
        percentage = "({0:.1f}%)".format(100*clinched/total)
    return "{0:.2f}".format(clinched) + " of {0:.2f}".format(total) + \
        " mi " + percentage

class GraphListEntry:
    """This class encapsulates information about generated graphs for
    inclusion in the DB table.  Field names here match column names
    in the "graphs" DB table.
    """

    def __init__(self,filename,descr,vertices,edges,format,category):
        self.filename = filename
        self.descr = descr
        self.vertices = vertices
        self.edges = edges
        self.format = format
        self.category = category
    
# 
# Execution code starts here
#
# start a timer for including elapsed time reports in messages
et = ElapsedTime()

# create a ErrorList
el = ErrorList()

# argument parsing
#
parser = argparse.ArgumentParser(description="Create SQL, stats, graphs, and log files from highway and user data for the Travel Mapping project.")
parser.add_argument("-w", "--highwaydatapath", default="../../../HighwayData", \
                        help="path to the root of the highway data directory structure")
parser.add_argument("-s", "--systemsfile", default="systems.csv", \
                        help="file of highway systems to include")
parser.add_argument("-u", "--userlistfilepath", default="../../../UserData/list_files",\
                        help="path to the user list file data")
parser.add_argument("-d", "--databasename", default="TravelMapping", \
                        help="Database name for .sql file name")
parser.add_argument("-l", "--logfilepath", default=".", help="Path to write log files, which should have a \"users\" subdirectory")
parser.add_argument("-c", "--csvstatfilepath", default=".", help="Path to write csv statistics files")
parser.add_argument("-g", "--graphfilepath", default=".", help="Path to write graph format data files")
parser.add_argument("-k", "--skipgraphs", action="store_true", help="Turn off generation of graph files")
parser.add_argument("-n", "--nmpmergepath", default="", help="Path to write data with NMPs merged (generated only if specified)")
parser.add_argument("-U", "--userlist", default=None, nargs="+", help="For Development: list of users to use in dataset")
parser.add_argument("-t", "--numthreads", default="4", help="Number of threads to use for concurrent tasks")
parser.add_argument("-e", "--errorcheck", action="store_true", help="Run only the subset of the process needed to verify highway data changes")
args = parser.parse_args()

#
# Get list of travelers in the system
traveler_ids = args.userlist
traveler_ids = os.listdir(args.userlistfilepath) if traveler_ids is None else (id + ".list" for id in traveler_ids)

# number of threads to use
num_threads = int(args.numthreads)

# read region, country, continent descriptions
print(et.et() + "Reading region, country, and continent descriptions.")

continents = []
try:
    file = open(args.highwaydatapath+"/continents.csv", "rt",encoding='utf-8')
except OSError as e:
    el.add_error(str(e))
else:
    lines = file.readlines()
    file.close()
    lines.pop(0)  # ignore header line
    for line in lines:
        fields = line.rstrip('\n').split(";")
        if len(fields) != 2:
            el.add_error("Could not parse continents.csv line: " + line)
            continue
        continents.append(fields)

countries = []
try:
    file = open(args.highwaydatapath+"/countries.csv", "rt",encoding='utf-8')
except OSError as e:
    el.add_error(str(e))
else:
    lines = file.readlines()
    file.close()
    lines.pop(0)  # ignore header line
    for line in lines:
        fields = line.rstrip('\n').split(";")
        if len(fields) != 2:
            el.add_error("Could not parse countries.csv line: " + line)
            continue
        countries.append(fields)

all_regions = []
try:
    file = open(args.highwaydatapath+"/regions.csv", "rt",encoding='utf-8')
except OSError as e:
    el.add_error(str(e))
else:
    lines = file.readlines()
    file.close()
    lines.pop(0)  # ignore header line
    for line in lines:
        fields = line.rstrip('\n').split(";")
        if len(fields) != 5:
            el.add_error("Could not parse regions.csv line: " + line)
            continue
        # look up country and continent, add index into those arrays
        # in case they're needed for lookups later (not needed for DB)
        for i in range(len(countries)):
            country = countries[i][0]
            if country == fields[2]:
                fields.append(i)
                break
        if len(fields) != 6:
            el.add_error("Could not find country matching regions.csv line: " + line)
            continue
        for i in range(len(continents)):
            continent = continents[i][0]
            if continent == fields[3]:
                fields.append(i)
                break
        if len(fields) != 7:
            el.add_error("Could not find continent matching regions.csv line: " + line)
            continue
        all_regions.append(fields)

# Create a list of HighwaySystem objects, one per system in systems.csv file
highway_systems = []
print(et.et() + "Reading systems list in " + args.highwaydatapath+"/"+args.systemsfile + ".  ",end="",flush=True)
try:
    file = open(args.highwaydatapath+"/"+args.systemsfile, "rt",encoding='utf-8')
except OSError as e:
    el.add_error(str(e))
else:
    lines = file.readlines()
    file.close()
    lines.pop(0)  # ignore header line for now
    ignoring = []
    for line in lines:
        if line.startswith('#'):
            ignoring.append("Ignored comment in " + args.systemsfile + ": " + line.rstrip('\n'))
            continue
        fields = line.rstrip('\n').split(";")
        if len(fields) != 6:
            el.add_error("Could not parse " + args.systemsfile + " line: " + line)
            continue
        print(fields[0] + ".",end="",flush=True)
        hs = HighwaySystem(fields[0], fields[1],
                           fields[2].replace("'","''"),
                           fields[3], fields[4], fields[5], el,
                           args.highwaydatapath+"/hwy_data/_systems")
        highway_systems.append(hs)
    print("")
    # print at the end the lines ignored
    for line in ignoring:
        print(line)

# list for datacheck errors that we will need later
datacheckerrors = []

# check for duplicate root entries among Route and ConnectedRoute
# data in all highway systems
print(et.et() + "Checking for duplicate list names in routes, roots in routes and connected routes.",flush=True)
roots = set()
list_names = set()
duplicate_list_names = set()
for h in highway_systems:
    for r in h.route_list:
        if r.root in roots:
            el.add_error("Duplicate root in route lists: " + r.root)
        else:
            roots.add(r.root)
        list_name = r.region + ' ' + r.list_entry_name()
        if list_name in list_names:
            duplicate_list_names.add(list_name)
        else:
            list_names.add(list_name)
            
con_roots = set()
for h in highway_systems:
    for cr in h.con_route_list:
        for r in cr.roots:
            if r.root in con_roots:
                el.add_error("Duplicate root in con_route lists: " + r.root)
            else:
                con_roots.add(r.root)

# Make sure every route was listed as a part of some connected route
if len(roots) == len(con_roots):
    print("Check passed: same number of routes as connected route roots. " + str(len(roots)))
else:
    el.add_error("Check FAILED: " + str(len(roots)) + " routes != " + str(len(con_roots)) + " connected route roots.")
    roots = roots - con_roots
    # there will be some leftovers, let's look up their routes to make
    # an error report entry (not worried about efficiency as there would
    # only be a few in reasonable cases)
    num_found = 0
    for h in highway_systems:
        for r in h.route_list:
            for lr in roots:
                if lr == r.root:
                    el.add_error("route " + lr + " not matched by any connected route root.")
                    num_found += 1
                    break
    print("Added " + str(num_found) + " ROUTE_NOT_IN_CONNECTED error entries.")

# report any duplicate list names as errors
if len(duplicate_list_names) > 0:
    print("Found " + str(len(duplicate_list_names)) + " DUPLICATE_LIST_NAME case(s).")
    for d in duplicate_list_names:
        el.add_error("Duplicate list name: " + d)
else:
    print("No duplicate list names found.")

# write file mapping CHM datacheck route lists to root (commented out,
# unlikely needed now)
#print(et.et() + "Writing CHM datacheck to TravelMapping route pairings.")
#file = open(args.csvstatfilepath + "/routepairings.csv","wt")
#for h in highway_systems:
#    for r in h.route_list:
#        file.write(r.region + " " + r.list_entry_name() + ";" + r.root + "\n")
#file.close()

# For tracking whether any .wpt files are in the directory tree
# that do not have a .csv file entry that causes them to be
# read into the data
print(et.et() + "Finding all .wpt files. ",end="",flush=True)
all_wpt_files = []
for dir, sub, files in os.walk(args.highwaydatapath+"/hwy_data"):
    for file in files:
        if file.endswith('.wpt') and '_boundaries' not in dir:
            all_wpt_files.append(dir+"/"+file)
print(str(len(all_wpt_files)) + " files found.")

# For finding colocated Waypoints and concurrent segments, we have
# quadtree of all Waypoints in existence to find them efficiently
all_waypoints = WaypointQuadtree(-90,-180,90,180)
all_waypoints_lock = threading.Lock()

print(et.et() + "Reading waypoints for all routes.")
# Next, read all of the .wpt files for each HighwaySystem
def read_wpts_for_highway_system(h):
    print(h.systemname,end="",flush=True)
    for r in h.route_list:
        # get full path to remove from all_wpt_files list
        wpt_path = args.highwaydatapath+"/hwy_data"+"/"+r.region + "/" + r.system.systemname+"/"+r.root+".wpt"
        if wpt_path in all_wpt_files:
            all_wpt_files.remove(wpt_path)
        r.read_wpt(all_waypoints,all_waypoints_lock,datacheckerrors,
                   el,args.highwaydatapath+"/hwy_data")
        if len(r.point_list) < 2:
            el.add_error("Route contains fewer than 2 points: " + str(r))
        print(".", end="",flush=True)
        #print(str(r))
        #r.print_route()
    print("!", flush=True)

# set up for threaded processing of highway systems
class ReadWptThread(threading.Thread):

    def __init__(self, id, hs_list, lock):
        threading.Thread.__init__(self)
        self.id = id
        self.hs_list = hs_list
        self.lock = lock

    def run(self):
        #print("Starting ReadWptThread " + str(self.id) + " lock is " + str(self.lock))
        while True:
            self.lock.acquire(True)
            #print("Thread " + str(self.id) + " with len(self.hs_list)=" + str(len(self.hs_list)))
            if len(self.hs_list) == 0:
                self.lock.release()
                break
            h = self.hs_list.pop()
            self.lock.release()
            #print("Thread " + str(self.id) + " assigned " + str(h))
            read_wpts_for_highway_system(h)
                
        #print("Exiting ReadWptThread " + str(self.id))
        
hs_lock = threading.Lock()
#print("Created lock: " + str(hs_lock))
hs = highway_systems[:]
hs.reverse()
thread_list = []
# create threads
for i in range(num_threads):
    thread_list.append(ReadWptThread(i, hs, hs_lock))

# start threads
for t in thread_list:
    t.start()

# wait for threads
for t in thread_list:
    t.join()

#for h in highway_systems:
#    read_wpts_for_highway_system(h)

print(et.et() + "Sorting waypoints in Quadtree.")
all_waypoints.sort()

print(et.et() + "Sorting colocated point lists.")
for w in all_waypoints.point_list():
    if w.colocated is not None:
        w.colocated.sort(key=lambda waypoint: waypoint.route.root + "@" + waypoint.label)

print(et.et() + "Finding unprocessed wpt files.", flush=True)
unprocessedfile = open(args.logfilepath+'/unprocessedwpts.log','w',encoding='utf-8')
if len(all_wpt_files) > 0:
    print(str(len(all_wpt_files)) + " .wpt files in " + args.highwaydatapath +
          "/hwy_data not processed, see unprocessedwpts.log.")
    for file in all_wpt_files:
        unprocessedfile.write(file[file.find('hwy_data'):] + '\n')
else:
    print("All .wpt files in " + args.highwaydatapath +
          "/hwy_data processed.")
unprocessedfile.close()

# Near-miss point log
print(et.et() + "Near-miss point log and tm-master.nmp file.", flush=True)

# read in fp file
nmpfplist = []
nmpfpfile = open(args.highwaydatapath+'/nmpfps.log','r')
nmpfpfilelines = nmpfpfile.readlines()
for line in nmpfpfilelines:
    if len(line.rstrip('\n ')) > 0:
        nmpfplist.append(line.rstrip('\n '))
nmpfpfile.close()

nmploglines = []
nmplog = open(args.logfilepath+'/nearmisspoints.log','w')
nmpnmp = open(args.logfilepath+'/tm-master.nmp','w')
for w in all_waypoints.point_list():
    if w.near_miss_points is not None:
        nmpline = str(w) + " NMP "
        nmplooksintentional = False
        nmpnmplines = []
        # sort the near miss points for consistent ordering to facilitate
        # NMP FP marking
        for other_w in sorted(w.near_miss_points,
                              key=lambda waypoint:
                              waypoint.route.root + "@" + waypoint.label):
            if (abs(w.lat - other_w.lat) < 0.0000015) and \
               (abs(w.lng - other_w.lng) < 0.0000015):
                nmplooksintentional = True
            nmpline += str(other_w) + " "
            w_label = w.route.root + "@" + w.label
            other_label = other_w.route.root + "@" + other_w.label
            # make sure we only plot once, since the NMP should be listed
            # both ways (other_w in w's list, w in other_w's list)
            if w_label < other_label:
                nmpnmplines.append(w_label + " " + str(w.lat) + " " + str(w.lng))
                nmpnmplines.append(other_label + " " + str(other_w.lat) + " " + str(other_w.lng))
        # indicate if this was in the FP list or if it's off by exact amt
        # so looks like it's intentional, and detach near_miss_points list
        # so it doesn't get a rewrite in nmp_merged WPT files
        # also set the extra field to mark FP/LI items in the .nmp file
        extra_field = ""
        if nmpline.rstrip() in nmpfplist:
            nmpfplist.remove(nmpline.rstrip())
            nmpline += "[MARKED FP]"
            w.near_miss_points = None
            extra_field += "FP"
        if nmplooksintentional:
            nmpline += "[LOOKS INTENTIONAL]"
            w.near_miss_points = None
            extra_field += "LI"
        if extra_field != "":
            extra_field = " " + extra_field
        nmploglines.append(nmpline.rstrip())

        # write actual lines to .nmp file, indicating FP and/or LI
        # for marked FPs or looks intentional items
        for nmpnmpline in nmpnmplines:
            nmpnmp.write(nmpnmpline + extra_field + "\n")
nmpnmp.close()

# sort and write actual lines to nearmisspoints.log
nmploglines.sort()
for n in nmploglines:
    nmplog.write(n + '\n')
nmploglines = None
nmplog.close()

# report any unmatched nmpfps.log entries
nmpfpsunmatchedfile = open(args.logfilepath+'/nmpfpsunmatched.log','w')
for line in nmpfplist:
    nmpfpsunmatchedfile.write(line + '\n')
nmpfpsunmatchedfile.close()

# if requested, rewrite data with near-miss points merged in
if args.nmpmergepath != "" and not args.errorcheck:
    print(et.et() + "Writing near-miss point merged wpt files.", flush=True)
    for h in highway_systems:
        print(h.systemname, end="", flush=True)
        for r in h.route_list:
            wptpath = args.nmpmergepath + "/" + r.region + "/" + h.systemname
            os.makedirs(wptpath, exist_ok=True)
            wptfile = open(wptpath + "/" + r.root + ".wpt", "wt")
            for w in r.point_list:
                wptfile.write(w.label + ' ')
                for a in w.alt_labels:
                    wptfile.write(a + ' ')
                if w.near_miss_points is None:
                    wptfile.write("http://www.openstreetmap.org/?lat={0:.6f}".format(w.lat) + "&lon={0:.6f}".format(w.lng) + "\n")
                else:
                    # for now, arbitrarily choose the northernmost
                    # latitude and easternmost longitude values in the
                    # list and denote a "merged" point with the https
                    lat = w.lat
                    lng = w.lng
                    for other_w in w.near_miss_points:
                        if other_w.lat > lat:
                            lat = other_w.lat
                        if other_w.lng > lng:
                            lng = other_w.lng
                    wptfile.write("https://www.openstreetmap.org/?lat={0:.6f}".format(lat) + "&lon={0:.6f}".format(lng) + "\n")

            wptfile.close()
        print(".", end="", flush=True)
    print()

# Create hash table for faster lookup of routes by list file name
print(et.et() + "Creating route hash table for list processing:",flush=True)
route_hash = dict()
for h in highway_systems:
    for r in h.route_list:
        route_hash[(r.region + ' ' + r.list_entry_name()).lower()] = r
        for a in r.alt_route_names:
            route_hash[(r.region + ' ' + a).lower()] = r

# Create a list of TravelerList objects, one per person
traveler_lists = []

print(et.et() + "Processing traveler list files:",end="",flush=True)
for t in traveler_ids:
    if t.endswith('.list'):
        print(" " + t,end="",flush=True)
        traveler_lists.append(TravelerList(t,route_hash,args.userlistfilepath))
print(" processed " + str(len(traveler_lists)) + " traveler list files.")
traveler_lists.sort(key=lambda TravelerList: TravelerList.traveler_name)

# Read updates.csv file, just keep in the fields array for now since we're
# just going to drop this into the DB later anyway
updates = []
print(et.et() + "Reading updates file.  ",end="",flush=True)
with open(args.highwaydatapath+"/updates.csv", "rt", encoding='UTF-8') as file:
    lines = file.readlines()
file.close()

lines.pop(0)  # ignore header line
for line in lines:
    fields = line.rstrip('\n').split(';')
    if len(fields) != 5:
        print("Could not parse updates.csv line: " + line)
        continue
    updates.append(fields)
print("")

# Same plan for systemupdates.csv file, again just keep in the fields
# array for now since we're just going to drop this into the DB later
# anyway
systemupdates = []
print(et.et() + "Reading systemupdates file.  ",end="",flush=True)
with open(args.highwaydatapath+"/systemupdates.csv", "rt", encoding='UTF-8') as file:
    lines = file.readlines()
file.close()

lines.pop(0)  # ignore header line
for line in lines:
    fields = line.rstrip('\n').split(';')
    if len(fields) != 5:
        print("Could not parse systemupdates.csv line: " + line)
        continue
    systemupdates.append(fields)
print("")

# write log file for points in use -- might be more useful in the DB later,
# or maybe in another format
print(et.et() + "Writing points in use log.")
inusefile = open(args.logfilepath+'/pointsinuse.log','w',encoding='UTF-8')
inusefile.write("Log file created at: " + str(datetime.datetime.now()) + "\n")
for h in highway_systems:
    for r in h.route_list:
        if len(r.labels_in_use) > 0:
            inusefile.write(r.root + "(" + str(len(r.point_list)) + "):")
            for label in sorted(r.labels_in_use):
                inusefile.write(" " + label)
            inusefile.write("\n")
            r.labels_in_use = None
inusefile.close()

# write log file for alt labels not in use
print(et.et() + "Writing unused alt labels log.")
unusedfile = open(args.logfilepath+'/unusedaltlabels.log','w',encoding='UTF-8')
unusedfile.write("Log file created at: " + str(datetime.datetime.now()) + "\n")
total_unused_alt_labels = 0
for h in highway_systems:
    for r in h.route_list:
        if len(r.unused_alt_labels) > 0:
            total_unused_alt_labels += len(r.unused_alt_labels)
            unusedfile.write(r.root + "(" + str(len(r.unused_alt_labels)) + "):")
            for label in sorted(r.unused_alt_labels):
                unusedfile.write(" " + label)
            unusedfile.write("\n")
            r.unused_alt_labels = None
unusedfile.write("Total: " + str(total_unused_alt_labels) + "\n")
unusedfile.close()


# concurrency detection -- will augment our structure with list of concurrent
# segments with each segment (that has a concurrency)
print(et.et() + "Concurrent segment detection.",end="",flush=True)
concurrencyfile = open(args.logfilepath+'/concurrencies.log','w',encoding='UTF-8')
concurrencyfile.write("Log file created at: " + str(datetime.datetime.now()) + "\n")
for h in highway_systems:
    print(".",end="",flush=True)
    for r in h.route_list:
        for s in r.segment_list:
            if s.waypoint1.colocated is not None and s.waypoint2.colocated is not None:
                for w1 in s.waypoint1.colocated:
                    if w1.route is not r:
                        for w2 in s.waypoint2.colocated:
                            if w1.route is w2.route:
                                other = w1.route.find_segment_by_waypoints(w1,w2)
                                if other is not None:
                                    if s.concurrent is None:
                                        s.concurrent = []
                                        other.concurrent = s.concurrent
                                        s.concurrent.append(s)
                                        s.concurrent.append(other)
                                        concurrencyfile.write("New concurrency [" + str(s) + "][" + str(other) + "] (" + str(len(s.concurrent)) + ")\n")
                                    else:
                                        other.concurrent = s.concurrent
                                        if other not in s.concurrent:
                                            s.concurrent.append(other)
                                            #concurrencyfile.write("Added concurrency [" + str(s) + "]-[" + str(other) + "] ("+ str(len(s.concurrent)) + ")\n")
                                            concurrencyfile.write("Extended concurrency ")
                                            for x in s.concurrent:
                                                concurrencyfile.write("[" + str(x) + "]")
                                            concurrencyfile.write(" (" + str(len(s.concurrent)) + ")\n")
print("!")

# now augment any traveler clinched segments for concurrencies

print(et.et() + "Augmenting travelers for detected concurrent segments.",end="",flush=True)
for t in traveler_lists:
    print(".",end="",flush=True)
    for s in t.clinched_segments:
        if s.concurrent is not None:
            for hs in s.concurrent:
                if hs.route.system.active_or_preview() and hs.add_clinched_by(t):
                    concurrencyfile.write("Concurrency augment for traveler " + t.traveler_name + ": [" + str(hs) + "] based on [" + str(s) + "]\n")
print("!")
concurrencyfile.close()

# compute lots of stats, first total mileage by route, system, overall, where
# system and overall are stored in dictionaries by region
print(et.et() + "Computing stats.",end="",flush=True)
# now also keeping separate totals for active only, active+preview,
# and all for overall (not needed for system, as a system falls into just
# one of these categories)
active_only_mileage_by_region = dict()
active_preview_mileage_by_region = dict()
overall_mileage_by_region = dict()
for h in highway_systems:
    print(".",end="",flush=True)
    for r in h.route_list:
        for s in r.segment_list:
            segment_length = s.length()
            # always add the segment mileage to the route
            r.mileage += segment_length
            # but we do need to check for concurrencies for others
            system_concurrency_count = 1
            active_only_concurrency_count = 1
            active_preview_concurrency_count = 1
            overall_concurrency_count = 1
            if s.concurrent is not None:
                for other in s.concurrent:
                    if other != s:
                        overall_concurrency_count += 1
                        if other.route.system.active_or_preview():
                            active_preview_concurrency_count += 1
                            if other.route.system.active():
                                active_only_concurrency_count += 1
                        if other.route.system == r.system:
                            system_concurrency_count += 1
            # we know how many times this segment will be encountered
            # in both the system and overall/active+preview/active-only
            # routes, so let's add in the appropriate (possibly fractional)
            # mileage to the overall totals and to the system categorized
            # by its region
            #
            # first, overall mileage for this region, add to overall
            # if an entry already exists, create entry if not
            if r.region in overall_mileage_by_region:
                overall_mileage_by_region[r.region] = overall_mileage_by_region[r.region] + \
                    segment_length/overall_concurrency_count
            else:
                overall_mileage_by_region[r.region] = segment_length/overall_concurrency_count

            # next, same thing for active_preview mileage for the region,
            # if active or preview
            if r.system.active_or_preview():
                if r.region in active_preview_mileage_by_region:
                    active_preview_mileage_by_region[r.region] = active_preview_mileage_by_region[r.region] + \
                    segment_length/active_preview_concurrency_count
                else:
                    active_preview_mileage_by_region[r.region] = segment_length/active_preview_concurrency_count

            # now same thing for active_only mileage for the region,
            # if active
            if r.system.active():
                if r.region in active_only_mileage_by_region:
                    active_only_mileage_by_region[r.region] = active_only_mileage_by_region[r.region] + \
                    segment_length/active_only_concurrency_count
                else:
                    active_only_mileage_by_region[r.region] = segment_length/active_only_concurrency_count

            # now we move on to totals by region, only the
            # overall since an entire highway system must be
            # at the same level
            if r.region in h.mileage_by_region:
                h.mileage_by_region[r.region] = h.mileage_by_region[r.region] + \
                        segment_length/system_concurrency_count
            else:
                h.mileage_by_region[r.region] = segment_length/system_concurrency_count

            # that's it for overall stats, now credit all travelers
            # who have clinched this segment in their stats
            for t in s.clinched_by:
                # credit active+preview for this region, which it must be
                # if this segment is clinched by anyone but still check
                # in case a concurrency detection might otherwise credit
                # a traveler with miles in a devel system
                if r.system.active_or_preview():
                    if r.region in t.active_preview_mileage_by_region:
                        t.active_preview_mileage_by_region[r.region] = t.active_preview_mileage_by_region[r.region] + \
                            segment_length/active_preview_concurrency_count
                    else:
                        t.active_preview_mileage_by_region[r.region] = segment_length/active_preview_concurrency_count

                # credit active only for this region
                if r.system.active():
                    if r.region in t.active_only_mileage_by_region:
                        t.active_only_mileage_by_region[r.region] = t.active_only_mileage_by_region[r.region] + \
                            segment_length/active_only_concurrency_count
                    else:
                        t.active_only_mileage_by_region[r.region] = segment_length/active_only_concurrency_count


                # credit this system in this region in the messy dictionary
                # of dictionaries, but skip devel system entries
                if r.system.active_or_preview():
                    if h.systemname not in t.system_region_mileages:
                        t.system_region_mileages[h.systemname] = dict()
                    t_system_dict = t.system_region_mileages[h.systemname]
                    if r.region in t_system_dict:
                        t_system_dict[r.region] = t_system_dict[r.region] + \
                        segment_length/system_concurrency_count
                    else:
                        t_system_dict[r.region] = segment_length/system_concurrency_count
print("!", flush=True)

print(et.et() + "Writing highway data stats log file (highwaydatastats.log).",flush=True)
hdstatsfile = open(args.logfilepath+"/highwaydatastats.log","wt",encoding='UTF-8')
hdstatsfile.write("Travel Mapping highway mileage as of " + str(datetime.datetime.now()) + '\n')
active_only_miles = math.fsum(list(active_only_mileage_by_region.values()))
hdstatsfile.write("Active routes (active): " + "{0:.2f}".format(active_only_miles) + " mi\n")
active_preview_miles = math.fsum(list(active_preview_mileage_by_region.values()))
hdstatsfile.write("Clinchable routes (active, preview): " + "{0:.2f}".format(active_preview_miles) + " mi\n")
overall_miles = math.fsum(list(overall_mileage_by_region.values()))
hdstatsfile.write("All routes (active, preview, devel): " + "{0:.2f}".format(overall_miles) + " mi\n")
hdstatsfile.write("Breakdown by region:\n")
# let's sort alphabetically by region instead of using whatever order
# comes out of the dictionary
# a nice enhancement later here might break down by continent, then country,
# then region
region_entries = []
for region in list(overall_mileage_by_region.keys()):
    # look up active+preview and active-only mileages if they exist
    if region in list(active_preview_mileage_by_region.keys()):
        region_active_preview_miles = active_preview_mileage_by_region[region]
    else:
        region_active_preview_miles = 0.0
    if region in list(active_only_mileage_by_region.keys()):
        region_active_only_miles = active_only_mileage_by_region[region]
    else:
        region_active_only_miles = 0.0

    region_entries.append(region + ": " +
                          "{0:.2f}".format(region_active_only_miles) + " (active), " +
                          "{0:.2f}".format(region_active_preview_miles) + " (active, preview) " +
                          "{0:.2f}".format(overall_mileage_by_region[region]) + " (active, preview, devel)\n")
region_entries.sort()
for e in region_entries:
    hdstatsfile.write(e)

for h in highway_systems:
    hdstatsfile.write("System " + h.systemname + " (" + h.level + ") total: "
                      + "{0:.2f}".format(math.fsum(list(h.mileage_by_region.values()))) \
                             + ' mi\n')
    if len(h.mileage_by_region) > 1:
        hdstatsfile.write("System " + h.systemname + " by region:\n")
        for region in sorted(h.mileage_by_region.keys()):
            hdstatsfile.write(region + ": " + "{0:.2f}".format(h.mileage_by_region[region]) + " mi\n")
    hdstatsfile.write("System " + h.systemname + " by route:\n")
    for cr in h.con_route_list:
        con_total_miles = 0.0
        to_write = ""
        for r in cr.roots:
            to_write += "  " + r.readable_name() + ": " + "{0:.2f}".format(r.mileage) + " mi\n"
            con_total_miles += r.mileage
        cr.mileage = con_total_miles
        hdstatsfile.write(cr.readable_name() + ": " + "{0:.2f}".format(con_total_miles) + " mi")
        if len(cr.roots) == 1:
            hdstatsfile.write(" (" + cr.roots[0].readable_name() + " only)\n")
        else:
            hdstatsfile.write("\n" + to_write)

hdstatsfile.close()
# this will be used to store DB entry lines for clinchedSystemMileageByRegion
# table as needed values are computed here, to be added into the DB
# later in the program
csmbr_values = []
# and similar for DB entry lines for clinchedConnectedRoutes table
# and clinchedRoutes table
ccr_values = []
cr_values = []
# now add user clinched stats to their log entries
print(et.et() + "Creating per-traveler stats log entries and augmenting data structure.",end="",flush=True)
for t in traveler_lists:
    print(".",end="",flush=True)
    t.log_entries.append("Clinched Highway Statistics")
    t_active_only_miles = math.fsum(list(t.active_only_mileage_by_region.values()))
    t.log_entries.append("Overall in active systems: " + format_clinched_mi(t_active_only_miles,active_only_miles))
    t_active_preview_miles = math.fsum(list(t.active_preview_mileage_by_region.values()))
    t.log_entries.append("Overall in active+preview systems: " + format_clinched_mi(t_active_preview_miles,active_preview_miles))

    t.log_entries.append("Overall by region: (each line reports active only then active+preview)")
    for region in sorted(t.active_preview_mileage_by_region.keys()):
        t_active_miles = 0.0
        total_active_miles = 0.0
        if region in list(t.active_only_mileage_by_region.keys()):
            t_active_miles = t.active_only_mileage_by_region[region]
            total_active_miles = active_only_mileage_by_region[region]
        t.log_entries.append(region + ": " +
                             format_clinched_mi(t_active_miles, total_active_miles) +
                             ", " +
                             format_clinched_mi(t.active_preview_mileage_by_region[region],
                                                active_preview_mileage_by_region[region]))

    t.active_systems_traveled = 0
    t.active_systems_clinched = 0
    t.preview_systems_traveled = 0
    t.preview_systems_clinched = 0
    active_systems = 0
    preview_systems = 0
    # "traveled" dictionaries indexed by system name, then conn or regular
    # route in another dictionary with keys route, values mileage
    # "clinched" dictionaries indexed by system name, values clinch count
    t.con_routes_traveled = dict()
    t.con_routes_clinched = dict()
    t.routes_traveled = dict()
    #t.routes_clinched = dict()

    # present stats by system here, also generate entries for
    # DB table clinchedSystemMileageByRegion as we compute and
    # have the data handy
    for h in highway_systems:
        if h.active_or_preview():
            if h.active():
                active_systems += 1
            else:
                preview_systems += 1
            t_system_overall = 0.0
            if h.systemname in t.system_region_mileages:
                t_system_overall = math.fsum(list(t.system_region_mileages[h.systemname].values()))
            t.log_entries.append("System " + h.systemname + " (" + h.level +
                                 ") overall: " +
                                 format_clinched_mi(t_system_overall, math.fsum(list(h.mileage_by_region.values()))))
            if t_system_overall > 0.0:
                if h.active():
                    t.active_systems_traveled += 1
                else:
                    t.preview_systems_traveled += 1
            if t_system_overall == math.fsum(list(h.mileage_by_region.values())):
                if h.active():
                    t.active_systems_clinched += 1
                else:
                    t.preview_systems_clinched += 1

            # stats by region covered by system, always in csmbr for
            # the DB, but add to logs only if it's been traveled at
            # all and it covers multiple regions
            if t_system_overall > 0.0:
                if len(h.mileage_by_region) > 1:
                    t.log_entries.append("System " + h.systemname + " by region:")
                for region in sorted(h.mileage_by_region.keys()):
                    system_region_mileage = 0.0
                    if h.systemname in t.system_region_mileages and region in t.system_region_mileages[h.systemname]:
                        system_region_mileage = t.system_region_mileages[h.systemname][region]
                        csmbr_values.append("('" + h.systemname + "','" + region + "','"
                                            + t.traveler_name + "','" +
                                            str(system_region_mileage) + "')")
                    if len(h.mileage_by_region) > 1:
                        t.log_entries.append("  " + region + ": " + \
                                                 format_clinched_mi(system_region_mileage, h.mileage_by_region[region]))

            # stats by highway for the system, by connected route and
            # by each segment crossing region boundaries if applicable
            if t_system_overall > 0.0:
                system_con_dict = dict()
                t.con_routes_traveled[h.systemname] = system_con_dict
                con_routes_clinched = 0
                t.log_entries.append("System " + h.systemname + " by route (traveled routes only):")
                for cr in h.con_route_list:
                    con_total_miles = 0.0
                    con_clinched_miles = 0.0
                    to_write = ""
                    for r in cr.roots:
                        # find traveled mileage on this by this user
                        miles = r.clinched_by_traveler(t)
                        if miles > 0.0:
                            if miles >= r.mileage:
                                clinched = '1'
                            else:
                                clinched = '0'
                            cr_values.append("('" + r.root + "','" + t.traveler_name + "','" +
                                             str(miles) + "','" + clinched + "')")
                            t.routes_traveled[r] = miles
                            con_clinched_miles += miles
                            to_write += "  " + r.readable_name() + ": " + \
                                format_clinched_mi(miles,r.mileage) + "\n"
                        con_total_miles += r.mileage
                    if con_clinched_miles > 0:
                        system_con_dict[cr] = con_clinched_miles
                        clinched = '0'
                        if con_clinched_miles == con_total_miles:
                            con_routes_clinched += 1
                            clinched = '1'
                        ccr_values.append("('" + cr.roots[0].root + "','" + t.traveler_name
                                          + "','" + str(con_clinched_miles) + "','"
                                          + clinched + "')")
                        t.log_entries.append(cr.readable_name() + ": " + \
                                             format_clinched_mi(con_clinched_miles,con_total_miles))
                        if len(cr.roots) == 1:
                            t.log_entries.append(" (" + cr.roots[0].readable_name() + " only)")
                        else:
                            t.log_entries.append(to_write)
                t.con_routes_clinched[h.systemname] = con_routes_clinched
                t.log_entries.append("System " + h.systemname + " connected routes traveled: " + \
                                         str(len(system_con_dict)) + " of " + \
                                         str(len(h.con_route_list)) + \
                                         " ({0:.1f}%)".format(100*len(system_con_dict)/len(h.con_route_list)) + \
                                         ", clinched: " + str(con_routes_clinched) + " of " + \
                                         str(len(h.con_route_list)) + \
                                         " ({0:.1f}%)".format(100*con_routes_clinched/len(h.con_route_list)) + \
                                         ".")


    # grand summary, active only
    t.log_entries.append("Traveled " + str(t.active_systems_traveled) + " of " + str(active_systems) +
                         " ({0:.1f}%)".format(100*t.active_systems_traveled/active_systems) +
                         ", Clinched " + str(t.active_systems_clinched) + " of " + str(active_systems) +
                         " ({0:.1f}%)".format(100*t.active_systems_clinched/active_systems) +
                         " active systems")
    # grand summary, active+preview
    t.log_entries.append("Traveled " + str(t.preview_systems_traveled) + " of " + str(preview_systems) +
                         " ({0:.1f}%)".format(100*t.preview_systems_traveled/preview_systems) +
                         ", Clinched " + str(t.preview_systems_clinched) + " of " + str(preview_systems) +
                         " ({0:.1f}%)".format(100*t.preview_systems_clinched/preview_systems) +
                         " preview systems")
print("!", flush=True)

# write log files for traveler lists
print(et.et() + "Writing traveler list logs.",flush=True)
for t in traveler_lists:
    t.write_log(args.logfilepath+"/users")

# write stats csv files
print(et.et() + "Writing stats csv files.",flush=True)

# first, overall per traveler by region, both active only and active+preview
allfile = open(args.csvstatfilepath + "/allbyregionactiveonly.csv","w",encoding='UTF-8')
allfile.write("Traveler,Total")
regions = sorted(active_only_mileage_by_region.keys())
for region in regions:
    allfile.write(',' + region)
allfile.write('\n')
for t in traveler_lists:
    allfile.write(t.traveler_name + ",{0:.2f}".format(math.fsum(list(t.active_only_mileage_by_region.values()))))
    for region in regions:
        if region in t.active_only_mileage_by_region.keys():
            allfile.write(',{0:.2f}'.format(t.active_only_mileage_by_region[region]))
        else:
            allfile.write(',0')
    allfile.write('\n')
allfile.write('TOTAL,{0:.2f}'.format(math.fsum(list(active_only_mileage_by_region.values()))))
for region in regions:
    allfile.write(',{0:.2f}'.format(active_only_mileage_by_region[region]))
allfile.write('\n')
allfile.close()

# active+preview
allfile = open(args.csvstatfilepath + "/allbyregionactivepreview.csv","w",encoding='UTF-8')
allfile.write("Traveler,Total")
regions = sorted(active_preview_mileage_by_region.keys())
for region in regions:
    allfile.write(',' + region)
allfile.write('\n')
for t in traveler_lists:
    allfile.write(t.traveler_name + ",{0:.2f}".format(math.fsum(list(t.active_preview_mileage_by_region.values()))))
    for region in regions:
        if region in t.active_preview_mileage_by_region.keys():
            allfile.write(',{0:.2f}'.format(t.active_preview_mileage_by_region[region]))
        else:
            allfile.write(',0')
    allfile.write('\n')
allfile.write('TOTAL,{0:.2f}'.format(math.fsum(list(active_preview_mileage_by_region.values()))))
for region in regions:
    allfile.write(',{0:.2f}'.format(active_preview_mileage_by_region[region]))
allfile.write('\n')
allfile.close()

# now, a file for each system, again per traveler by region
for h in highway_systems:
    sysfile = open(args.csvstatfilepath + "/" + h.systemname + '-all.csv',"w",encoding='UTF-8')
    sysfile.write('Traveler,Total')
    regions = sorted(h.mileage_by_region.keys())
    for region in regions:
        sysfile.write(',' + region)
    sysfile.write('\n')
    for t in traveler_lists:
        # only include entries for travelers who have any mileage in system
        if h.systemname in t.system_region_mileages:
            sysfile.write(t.traveler_name + ",{0:.2f}".format(math.fsum(list(t.system_region_mileages[h.systemname].values()))))
            for region in regions:
                if region in t.system_region_mileages[h.systemname]:
                    sysfile.write(',{0:.2f}'.format(t.system_region_mileages[h.systemname][region]))
                else:
                    sysfile.write(',0')
            sysfile.write('\n')
    sysfile.write('TOTAL,{0:.2f}'.format(math.fsum(list(h.mileage_by_region.values()))))
    for region in regions:
        sysfile.write(',{0:.2f}'.format(h.mileage_by_region[region]))
    sysfile.write('\n')
    sysfile.close()

# read in the datacheck false positives list
print(et.et() + "Reading datacheckfps.csv.",flush=True)
with open(args.highwaydatapath+"/datacheckfps.csv", "rt",encoding='utf-8') as file:
    lines = file.readlines()
file.close()

lines.pop(0)  # ignore header line
datacheckfps = []
datacheck_always_error = [ 'DUPLICATE_LABEL', 'HIDDEN_TERMINUS',
                           'LABEL_INVALID_CHAR', 'LABEL_SLASHES',
                           'LONG_UNDERSCORE', 'MALFORMED_URL',
                           'NONTERMINAL_UNDERSCORE' ]
for line in lines:
    fields = line.rstrip('\n').split(';')
    if len(fields) != 6:
        el.add_error("Could not parse datacheckfps.csv line: " + line)
        continue
    if fields[4] in datacheck_always_error:
        print("datacheckfps.csv line not allowed (always error): " + line)
        continue
    datacheckfps.append(fields)

# See if we have any errors that should be fatal to the site update process
if len(el.error_list) > 0:
    print("ABORTING due to " + str(len(el.error_list)) + " errors:")
    for i in range(len(el.error_list)):
        print(str(i+1) + ": " + el.error_list[i])
    sys.exit(1)

# Build a graph structure out of all highway data in active and
# preview systems
print(et.et() + "Setting up for graphs of highway data.", flush=True)
graph_data = HighwayGraph(all_waypoints, highway_systems, datacheckerrors)

print(et.et() + "Writing graph waypoint simplification log.", flush=True)
logfile = open(args.logfilepath + '/waypointsimplification.log', 'w')
for line in graph_data.waypoint_naming_log:
    logfile.write(line + '\n')
logfile.close()
graph_data.waypoint_naming_log = None

# create list of graph information for the DB
graph_list = []
graph_types = []
    
# start generating graphs and making entries for graph DB table

if args.skipgraphs or args.errorcheck:
    print(et.et() + "SKIPPING generation of subgraphs.", flush=True)
else:
    print(et.et() + "Writing master TM simple graph file, tm-master-simple.tmg", flush=True)
    (sv, se) = graph_data.write_master_tmg_simple(args.graphfilepath+'/tm-master-simple.tmg')
    graph_list.append(GraphListEntry('tm-master-simple.tmg', 'All Travel Mapping Data', sv, se, 'simple', 'master'))
    print(et.et() + "Writing master TM collapsed graph file, tm-master.tmg.", flush=True)
    (cv, ce) = graph_data.write_master_tmg_collapsed(args.graphfilepath+'/tm-master.tmg')
    graph_list.append(GraphListEntry('tm-master.tmg', 'All Travel Mapping Data', cv, ce, 'collapsed', 'master'))
    graph_types.append(['master', 'All Travel Mapping Data',
                        'These graphs contain all routes currently plotted in the Travel Mapping project.'])

    # graphs restricted by place/area - from areagraphs.csv file
    print("\n" + et.et() + "Creating area data graphs.", flush=True)
    with open(args.highwaydatapath+"/graphs/areagraphs.csv", "rt",encoding='utf-8') as file:
        lines = file.readlines()
    file.close()
    lines.pop(0);  # ignore header line
    area_list = []
    for line in lines:
        fields = line.rstrip('\n').split(";")
        if len(fields) != 5:
            print("Could not parse areagraphs.csv line: " + line)
            continue
        area_list.append(PlaceRadius(*fields))

    for a in area_list:
        print(a.base + '(' + str(a.r) + ') ', end="", flush=True)
        graph_data.write_subgraphs_tmg(graph_list, args.graphfilepath + "/", a.base + str(a.r) + "-area",
                                       a.place + " (" + str(a.r) + " mi radius)", "area", None, None, a)
    graph_types.append(['area', 'Routes Within a Given Radius of a Place',
                        'These graphs contain all routes currently plotted within the given distance radius of the given place.'])
    print("!")
        
    # Graphs restricted by region
    print(et.et() + "Creating regional data graphs.", flush=True)

    # We will create graph data and a graph file for each region that includes
    # any active or preview systems
    for r in all_regions:
        region_code = r[0]
        if region_code not in active_preview_mileage_by_region:
            continue
        region_name = r[1]
        region_type = r[4]
        print(region_code + ' ', end="",flush=True)
        graph_data.write_subgraphs_tmg(graph_list, args.graphfilepath + "/", region_code + "-region",
                                       region_name + " (" + region_type + ")", "region", [ region_code ], None, None)
    graph_types.append(['region', 'Routes Within a Single Region',
                        'These graphs contain all routes currently plotted within the given region.'])
    print("!")

    # Graphs restricted by system - from systemgraphs.csv file
    print(et.et() + "Creating system data graphs.", flush=True)

    # We will create graph data and a graph file for only a few interesting
    # systems, as many are not useful on their own
    h = None
    with open(args.highwaydatapath+"/graphs/systemgraphs.csv", "rt",encoding='utf-8') as file:
        lines = file.readlines()
    file.close()
    lines.pop(0);  # ignore header line
    for hname in lines:
        h = None
        for hs in highway_systems:
            if hs.systemname == hname.strip():
                h = hs
                break
        if h is not None:
            print(h.systemname + ' ', end="",flush=True)
            graph_data.write_subgraphs_tmg(graph_list, args.graphfilepath + "/", h.systemname+"-system",
                                           h.systemname + " (" + h.fullname + ")", "system", None, [ h ], None)
    if h is not None:
        graph_types.append(['system', 'Routes Within a Single Highway System',
                            'These graphs contain the routes within a single highway system and are not restricted by region.'])
    print("!")

    # Some additional interesting graphs, the "multisystem" graphs
    print(et.et() + "Creating multisystem graphs.", flush=True)

    with open(args.highwaydatapath+"/graphs/multisystem.csv", "rt",encoding='utf-8') as file:
        lines = file.readlines()
    file.close()
    lines.pop(0);  # ignore header line
    for line in lines:
        fields = line.rstrip('\n').split(";")
        if len(fields) != 3:
            print("Could not parse multisystem.csv line: " + line)
            continue
        print(fields[1] + ' ', end="", flush=True)
        systems = []
        selected_systems = fields[2].split(",")
        for h in highway_systems:
            if h.systemname in selected_systems:
                systems.append(h)
        graph_data.write_subgraphs_tmg(graph_list, args.graphfilepath + "/", fields[1],
                                       fields[0], "multisystem", None, systems, None)
    graph_types.append(['multisystem', 'Routes Within Multiple Highway Systems',
                        'These graphs contain the routes within a set of highway systems.'])
    print("!")

    # Some additional interesting graphs, the "multiregion" graphs
    print(et.et() + "Creating multiregion graphs.", flush=True)

    with open(args.highwaydatapath+"/graphs/multiregion.csv", "rt",encoding='utf-8') as file:
        lines = file.readlines()
    file.close()
    lines.pop(0);  # ignore header line
    for line in lines:
        fields = line.rstrip('\n').split(";")
        if len(fields) != 3:
            print("Could not parse multiregion.csv line: " + line)
            continue
        print(fields[1] + ' ', end="", flush=True)
        region_list = []
        selected_regions = fields[2].split(",")
        for r in all_regions:
            if r[0] in selected_regions and r[0] in active_preview_mileage_by_region:
                region_list.append(r[0])
        graph_data.write_subgraphs_tmg(graph_list, args.graphfilepath + "/", fields[1],
                                       fields[0], "multiregion", region_list, None, None)
    graph_types.append(['multiregion', 'Routes Within Multiple Regions',
                        'These graphs contain the routes within a set of regions.'])
    print("!")

    # country graphs - we find countries that have regions
    # that have routes with active or preview mileage
    print(et.et() + "Creating country graphs.", flush=True)
    for c in countries:
        region_list = []
        for r in all_regions:
            # does it match this country and have routes?
            if c[0] == r[2] and r[0] in active_preview_mileage_by_region:
                region_list.append(r[0])
        # does it have at least two?  if none, no data, if 1 we already
        # generated a graph for that one region
        if len(region_list) >= 2:
            print(c[0] + " ", end="", flush=True)
            graph_data.write_subgraphs_tmg(graph_list, args.graphfilepath + "/", c[0] + "-country",
                                           c[1] + " All Routes in Country", "country", region_list, None, None)
    graph_types.append(['country', 'Routes Within a Single Multi-Region Country',
                        'These graphs contain the routes within a single country that is composed of multiple regions that contain plotted routes.  Countries consisting of a single region are represented by their regional graph.'])
    print("!")

    # continent graphs -- any continent with data will be created
    print(et.et() + "Creating continent graphs.", flush=True)
    for c in continents:
        region_list = []
        for r in all_regions:
            # does it match this continent and have routes?
            if c[0] == r[3] and r[0] in active_preview_mileage_by_region:
                region_list.append(r[0])
        # generate for any continent with at least 1 region with mileage
        if len(region_list) >= 1:
            print(c[0] + " ", end="", flush=True)
            graph_data.write_subgraphs_tmg(graph_list, args.graphfilepath + "/", c[0] + "-continent",
                                           c[1] + " All Routes on Continent", "continent", region_list, None, None)
    graph_types.append(['continent', 'Routes Within a Continent',
                        'These graphs contain the routes on a continent.'])
    print("!")

# data check: visit each system and route and check for various problems
print(et.et() + "Performing data checks.",end="",flush=True)
# perform most datachecks here (list initialized above)
for h in highway_systems:
    print(".",end="",flush=True)
    for r in h.route_list:
        # set to be used per-route to find label duplicates
        all_route_labels = set()
        # set of tuples to be used for finding duplicate coordinates
        coords_used = set()

        visible_distance = 0.0
        # note that we assume the first point will be visible in each route
        # so the following is simply a placeholder
        last_visible = None
        prev_w = None

        # look for hidden termini
        if r.point_list[0].is_hidden:
            datacheckerrors.append(DatacheckEntry(r,[r.point_list[0].label],'HIDDEN_TERMINUS'))
        if r.point_list[len(r.point_list)-1].is_hidden:
            datacheckerrors.append(DatacheckEntry(r,[r.point_list[len(r.point_list)-1].label],'HIDDEN_TERMINUS'))

        for w in r.point_list:
            # duplicate labels
            label_list = w.alt_labels.copy()
            label_list.append(w.label)
            for label in label_list:
                lower_label = label.lower().strip("+*")
                if lower_label in all_route_labels:
                    datacheckerrors.append(DatacheckEntry(r,[lower_label],"DUPLICATE_LABEL"))
                else:
                    all_route_labels.add(lower_label)

            # out-of-bounds coords
            if w.lat > 90 or w.lat < -90 or w.lng > 180 or w.lng < -180:
                datacheckerrors.append(DatacheckEntry(r,[w.label],'OUT_OF_BOUNDS',
                                                      "("+str(w.lat)+","+str(w.lng)+")"))

            # duplicate coordinates
            latlng = w.lat, w.lng
            if latlng in coords_used:
                for other_w in r.point_list:
                    if w == other_w:
                        break
                    if w.lat == other_w.lat and w.lng == other_w.lng and w.label != other_w.label:
                        labels = []
                        labels.append(other_w.label)
                        labels.append(w.label)
                        datacheckerrors.append(DatacheckEntry(r,labels,"DUPLICATE_COORDS",
                                                              "("+str(latlng[0])+","+str(latlng[1])+")"))
            else:
               coords_used.add(latlng)

            # visible distance update, and last segment length check
            if prev_w is not None:
                last_distance = w.distance_to(prev_w)
                visible_distance += last_distance
                if last_distance > 20.0:
                    labels = []
                    labels.append(prev_w.label)
                    labels.append(w.label)
                    datacheckerrors.append(DatacheckEntry(r,labels,'LONG_SEGMENT',
                                                          "{0:.2f}".format(last_distance)))

            if not w.is_hidden:
                # complete visible distance check, omit report for active
                # systems to reduce clutter
                if visible_distance > 10.0 and not h.active():
                    labels = []
                    labels.append(last_visible.label)
                    labels.append(w.label)
                    datacheckerrors.append(DatacheckEntry(r,labels,'VISIBLE_DISTANCE',
                                                          "{0:.2f}".format(visible_distance)))
                last_visible = w
                visible_distance = 0.0

                # looking for the route within the label
                #match_start = w.label.find(r.route)
                #if match_start >= 0:
                    # we have a potential match, just need to make sure if the route
                    # name ends with a number that the matched substring isn't followed
                    # by more numbers (e.g., NY50 is an OK label in NY5)
                #    if len(r.route) + match_start == len(w.label) or \
                #            not w.label[len(r.route) + match_start].isdigit():
                # partially complete "references own route" -- too many FP
                #or re.fullmatch('.*/'+r.route+'.*',w.label[w.label) :
                # first check for number match after a slash, if there is one
                selfref_found = False
                if '/' in w.label and r.route[-1].isdigit():
                    digit_starts = len(r.route)-1
                    while digit_starts >= 0 and r.route[digit_starts].isdigit():
                        digit_starts-=1
                    if w.label[w.label.index('/')+1:] == r.route[digit_starts+1:]:
                        selfref_found = True
                    if w.label[w.label.index('/')+1:] == r.route:
                        selfref_found = True
                    if '_' in w.label[w.label.index('/')+1:] and w.label[w.label.index('/')+1:w.label.rindex('_')] == r.route[digit_starts+1:]:
                        selfref_found = True
                    if '_' in w.label[w.label.index('/')+1:] and w.label[w.label.index('/')+1:w.label.rindex('_')] == r.route:
                        selfref_found = True

                # now the remaining checks
                if selfref_found or r.route+r.banner == w.label or re.fullmatch(r.route+r.banner+'[_/].*',w.label):
                    datacheckerrors.append(DatacheckEntry(r,[w.label],'LABEL_SELFREF'))

                # look for too many underscores in label
                if w.label.count('_') > 1:
                    datacheckerrors.append(DatacheckEntry(r,[w.label],'LABEL_UNDERSCORES'))

                # look for too many characters after underscore in label
                if '_' in w.label:
                    if w.label.index('_') < len(w.label) - 5:
                        datacheckerrors.append(DatacheckEntry(r,[w.label],'LONG_UNDERSCORE'))

                # look for too many slashes in label
                if w.label.count('/') > 1:
                    datacheckerrors.append(DatacheckEntry(r,[w.label],'LABEL_SLASHES'))

                # look for parenthesis balance in label
                if w.label.count('(') != w.label.count(')'):
                    datacheckerrors.append(DatacheckEntry(r,[w.label],'LABEL_PARENS'))

                # look for labels with invalid characters
                if not re.fullmatch('[a-zA-Z0-9()/\+\*_\-\.]+', w.label):
                    datacheckerrors.append(DatacheckEntry(r,[w.label],'LABEL_INVALID_CHAR'))
                for a in w.alt_labels:
                    if not re.fullmatch('[a-zA-Z0-9()/\+\*_\-\.]+', a):
                        datacheckerrors.append(DatacheckEntry(r,[a],'LABEL_INVALID_CHAR'))

                # look for labels with a slash after an underscore
                if '_' in w.label and '/' in w.label and \
                        w.label.index('/') > w.label.index('_'):
                    datacheckerrors.append(DatacheckEntry(r,[w.label],'NONTERMINAL_UNDERSCORE'))

                # look for I-xx with Bus instead of BL or BS
                if re.fullmatch('I\-[0-9]*Bus', w.label):
                    datacheckerrors.append(DatacheckEntry(r,[w.label],'BUS_WITH_I'))

                # look for labels that look like hidden waypoints but
                # which aren't hidden
                if re.fullmatch('X[0-9][0-9][0-9][0-9][0-9][0-9]', w.label):
                    datacheckerrors.append(DatacheckEntry(r,[w.label],'LABEL_LOOKS_HIDDEN'))

                # look for USxxxA but not USxxxAlt, B/Bus (others?)
                ##if re.fullmatch('US[0-9]+A.*', w.label) and not re.fullmatch('US[0-9]+Alt.*', w.label) or \
                ##   re.fullmatch('US[0-9]+B.*', w.label) and \
                ##   not (re.fullmatch('US[0-9]+Bus.*', w.label) or re.fullmatch('US[0-9]+Byp.*', w.label)):
                ##    datacheckerrors.append(DatacheckEntry(r,[w.label],'US_BANNER'))

            prev_w = w

        # angle check is easier with a traditional for loop and array indices
        for i in range(1, len(r.point_list)-1):
            #print("computing angle for " + str(r.point_list[i-1]) + ' ' + str(r.point_list[i]) + ' ' + str(r.point_list[i+1]))
            if r.point_list[i-1].same_coords(r.point_list[i]) or \
               r.point_list[i+1].same_coords(r.point_list[i]):
                labels = []
                labels.append(r.point_list[i-1].label)
                labels.append(r.point_list[i].label)
                labels.append(r.point_list[i+1].label)
                datacheckerrors.append(DatacheckEntry(r,labels,'BAD_ANGLE'))
            else:
                angle = r.point_list[i].angle(r.point_list[i-1],r.point_list[i+1])
                if angle > 135:
                    labels = []
                    labels.append(r.point_list[i-1].label)
                    labels.append(r.point_list[i].label)
                    labels.append(r.point_list[i+1].label)
                    datacheckerrors.append(DatacheckEntry(r,labels,'SHARP_ANGLE',
                                                          "{0:.2f}".format(angle)))
print("!", flush=True)
print(et.et() + "Found " + str(len(datacheckerrors)) + " datacheck errors.")

datacheckerrors.sort(key=lambda DatacheckEntry: str(DatacheckEntry))

# now mark false positives
print(et.et() + "Marking datacheck false positives.",end="",flush=True)
fpfile = open(args.logfilepath+'/nearmatchfps.log','w',encoding='utf-8')
fpfile.write("Log file created at: " + str(datetime.datetime.now()) + "\n")
toremove = []
counter = 0
fpcount = 0
for d in datacheckerrors:
    #print("Checking: " + str(d))
    counter += 1
    if counter % 1000 == 0:
        print(".", end="",flush=True)
    for fp in datacheckfps:
        #print("Comparing: " + str(d) + " to " + str(fp))
        if d.match_except_info(fp):
            if d.info == fp[5]:
                #print("Match!")
                d.fp = True
                fpcount += 1
                datacheckfps.remove(fp)
                break
            fpfile.write("FP_ENTRY: " + fp[0] + ';' + fp[1] + ';' + fp[2] + ';' + fp[3] + ';' + fp[4] + ';' + fp[5] + '\n')
            fpfile.write("CHANGETO: " + fp[0] + ';' + fp[1] + ';' + fp[2] + ';' + fp[3] + ';' + fp[4] + ';' + d.info + '\n')
fpfile.close()
print("!", flush=True)
print(et.et() + "Matched " + str(fpcount) + " FP entries.", flush=True)

# write log of unmatched false positives from the datacheckfps.csv
print(et.et() + "Writing log of unmatched datacheck FP entries.")
fpfile = open(args.logfilepath+'/unmatchedfps.log','w',encoding='utf-8')
fpfile.write("Log file created at: " + str(datetime.datetime.now()) + "\n")
if len(datacheckfps) > 0:
    for entry in datacheckfps:
        fpfile.write(entry[0] + ';' + entry[1] + ';' + entry[2] + ';' + entry[3] + ';' + entry[4] + ';' + entry[5] + '\n')
else:
    fpfile.write("No unmatched FP entries.")
fpfile.close()

# datacheck.log file
print(et.et() + "Writing datacheck.log")
logfile = open(args.logfilepath + '/datacheck.log', 'w')
logfile.write("Log file created at: " + str(datetime.datetime.now()) + "\n")
logfile.write("Datacheck errors that have been flagged as false positives are not included.\n")
logfile.write("These entries should be in a format ready to paste into datacheckfps.csv.\n")
logfile.write("Root;Waypoint1;Waypoint2;Waypoint3;Error;Info\n")
if len(datacheckerrors) > 0:
    for d in datacheckerrors:
        if not d.fp:
            logfile.write(str(d)+"\n")
else:
    logfile.write("No datacheck errors found.")
logfile.close()
    
if args.errorcheck:
    print(et.et() + "SKIPPING database file.")
else:
    print(et.et() + "Writing database file " + args.databasename + ".sql.")
    # Once all data is read in and processed, create a .sql file that will
    # create all of the DB tables to be used by other parts of the project
    sqlfile = open(args.databasename+'.sql','w',encoding='UTF-8')
    # Note: removed "USE" line, DB name must be specified on the mysql command line
    
    # we have to drop tables in the right order to avoid foreign key errors
    sqlfile.write('DROP TABLE IF EXISTS datacheckErrors;\n')
    sqlfile.write('DROP TABLE IF EXISTS clinchedConnectedRoutes;\n')
    sqlfile.write('DROP TABLE IF EXISTS clinchedRoutes;\n')
    sqlfile.write('DROP TABLE IF EXISTS clinchedOverallMileageByRegion;\n')
    sqlfile.write('DROP TABLE IF EXISTS clinchedSystemMileageByRegion;\n')
    sqlfile.write('DROP TABLE IF EXISTS overallMileageByRegion;\n')
    sqlfile.write('DROP TABLE IF EXISTS systemMileageByRegion;\n')
    sqlfile.write('DROP TABLE IF EXISTS clinched;\n')
    sqlfile.write('DROP TABLE IF EXISTS segments;\n')
    sqlfile.write('DROP TABLE IF EXISTS waypoints;\n')
    sqlfile.write('DROP TABLE IF EXISTS connectedRouteRoots;\n')
    sqlfile.write('DROP TABLE IF EXISTS connectedRoutes;\n')
    sqlfile.write('DROP TABLE IF EXISTS routes;\n')
    sqlfile.write('DROP TABLE IF EXISTS systems;\n')
    sqlfile.write('DROP TABLE IF EXISTS updates;\n')
    sqlfile.write('DROP TABLE IF EXISTS systemUpdates;\n')
    sqlfile.write('DROP TABLE IF EXISTS regions;\n')
    sqlfile.write('DROP TABLE IF EXISTS countries;\n')
    sqlfile.write('DROP TABLE IF EXISTS continents;\n')
    
    # first, continents, countries, and regions
    sqlfile.write('CREATE TABLE continents (code VARCHAR(3), name VARCHAR(15), PRIMARY KEY(code));\n')
    sqlfile.write('INSERT INTO continents VALUES\n')
    first = True
    for c in continents:
        if not first:
            sqlfile.write(",")
        first = False
        sqlfile.write("('" + c[0] + "','" + c[1] + "')\n")
    sqlfile.write(";\n")

    sqlfile.write('CREATE TABLE countries (code VARCHAR(3), name VARCHAR(32), PRIMARY KEY(code));\n')
    sqlfile.write('INSERT INTO countries VALUES\n')
    first = True
    for c in countries:
        if not first:
            sqlfile.write(",")
        first = False
        sqlfile.write("('" + c[0] + "','" + c[1].replace("'","''") + "')\n")
    sqlfile.write(";\n")

    sqlfile.write('CREATE TABLE regions (code VARCHAR(8), name VARCHAR(48), country VARCHAR(3), continent VARCHAR(3), regiontype VARCHAR(32), PRIMARY KEY(code), FOREIGN KEY (country) REFERENCES countries(code), FOREIGN KEY (continent) REFERENCES continents(code));\n')
    sqlfile.write('INSERT INTO regions VALUES\n')
    first = True
    for r in all_regions:
        if not first:
            sqlfile.write(",")
        first = False
        sqlfile.write("('" + r[0] + "','" + r[1].replace("'","''") + "','" + r[2] + "','" + r[3] + "','" + r[4] + "')\n")
    sqlfile.write(";\n")

    # next, a table of the systems, consisting of the system name in the
    # field 'name', the system's country code, its full name, the default
    # color for its mapping, a level (one of active, preview, devel), and
    # a boolean indicating if the system is active for mapping in the
    # project in the field 'active'
    sqlfile.write('CREATE TABLE systems (systemName VARCHAR(10), countryCode CHAR(3), fullName VARCHAR(60), color VARCHAR(16), level VARCHAR(10), tier INTEGER, csvOrder INTEGER, PRIMARY KEY(systemName));\n')
    sqlfile.write('INSERT INTO systems VALUES\n')
    first = True
    csvOrder = 0
    for h in highway_systems:
        if not first:
            sqlfile.write(",")
        first = False
        sqlfile.write("('" + h.systemname + "','" +  h.country + "','" +
                      h.fullname + "','" + h.color + "','" + h.level +
                      "','" + str(h.tier) + "','" + str(csvOrder) + "')\n")
        csvOrder += 1
    sqlfile.write(";\n")

    # next, a table of highways, with the same fields as in the first line
    sqlfile.write('CREATE TABLE routes (systemName VARCHAR(10), region VARCHAR(8), route VARCHAR(16), banner VARCHAR(6), abbrev VARCHAR(3), city VARCHAR(100), root VARCHAR(32), mileage FLOAT, rootOrder INTEGER, csvOrder INTEGER, PRIMARY KEY(root), FOREIGN KEY (systemName) REFERENCES systems(systemName));\n')
    sqlfile.write('INSERT INTO routes VALUES\n')
    first = True
    csvOrder = 0
    for h in highway_systems:
        for r in h.route_list:
            if not first:
                sqlfile.write(",")
            first = False
            sqlfile.write("(" + r.csv_line() + ",'" + str(csvOrder) + "')\n")
            csvOrder += 1
    sqlfile.write(";\n")

    # connected routes table, but only first "root" in each in this table
    sqlfile.write('CREATE TABLE connectedRoutes (systemName VARCHAR(10), route VARCHAR(16), banner VARCHAR(6), groupName VARCHAR(100), firstRoot VARCHAR(32), mileage FLOAT, csvOrder INTEGER, PRIMARY KEY(firstRoot), FOREIGN KEY (firstRoot) REFERENCES routes(root));\n')
    sqlfile.write('INSERT INTO connectedRoutes VALUES\n')
    first = True
    csvOrder = 0
    for h in highway_systems:
        for cr in h.con_route_list:
            if not first:
                sqlfile.write(",")
            first = False
            sqlfile.write("(" + cr.csv_line() + ",'" + str(csvOrder) + "')\n")
            csvOrder += 1
    sqlfile.write(";\n")

    # This table has remaining roots for any connected route
    # that connects multiple routes/roots
    sqlfile.write('CREATE TABLE connectedRouteRoots (firstRoot VARCHAR(32), root VARCHAR(32), FOREIGN KEY (firstRoot) REFERENCES connectedRoutes(firstRoot));\n')
    first = True
    for h in highway_systems:
        for cr in h.con_route_list:
            if len(cr.roots) > 1:
                for i in range(1,len(cr.roots)):
                    if first:
                        sqlfile.write('INSERT INTO connectedRouteRoots VALUES\n')
                    if not first:
                        sqlfile.write(",")
                    first = False
                    sqlfile.write("('" + cr.roots[0].root + "','" + cr.roots[i].root + "')\n")
    sqlfile.write(";\n")

    # Now, a table with raw highway route data: list of points, in order, that define the route
    sqlfile.write('CREATE TABLE waypoints (pointId INTEGER, pointName VARCHAR(20), latitude DOUBLE, longitude DOUBLE, root VARCHAR(32), PRIMARY KEY(pointId), FOREIGN KEY (root) REFERENCES routes(root));\n')
    point_num = 0
    for h in highway_systems:
        for r in h.route_list:
            sqlfile.write('INSERT INTO waypoints VALUES\n')
            first = True
            for w in r.point_list:
                if not first:
                    sqlfile.write(",")
                first = False
                w.point_num = point_num
                sqlfile.write("(" + w.csv_line(point_num) + ")\n")
                point_num+=1
            sqlfile.write(";\n")

    # Build indices to speed latitude/longitude joins for intersecting highway queries
    sqlfile.write('CREATE INDEX `latitude` ON waypoints(`latitude`);\n')
    sqlfile.write('CREATE INDEX `longitude` ON waypoints(`longitude`);\n')

    # Table of all HighwaySegments.
    sqlfile.write('CREATE TABLE segments (segmentId INTEGER, waypoint1 INTEGER, waypoint2 INTEGER, root VARCHAR(32), PRIMARY KEY (segmentId), FOREIGN KEY (waypoint1) REFERENCES waypoints(pointId), FOREIGN KEY (waypoint2) REFERENCES waypoints(pointId), FOREIGN KEY (root) REFERENCES routes(root));\n')
    segment_num = 0
    clinched_list = []
    for h in highway_systems:
        for r in h.route_list:
            sqlfile.write('INSERT INTO segments VALUES\n')
            first = True
            for s in r.segment_list:
                if not first:
                    sqlfile.write(",")
                first = False
                sqlfile.write("(" + s.csv_line(segment_num) + ")\n")
                for t in s.clinched_by:
                    clinched_list.append("'" + str(segment_num) + "','" + t.traveler_name + "'")
                segment_num += 1
            sqlfile.write(";\n")

    # maybe a separate traveler table will make sense but for now, I'll just use
    # the name from the .list name
    sqlfile.write('CREATE TABLE clinched (segmentId INTEGER, traveler VARCHAR(48), FOREIGN KEY (segmentId) REFERENCES segments(segmentId));\n')
    for start in range(0, len(clinched_list), 10000):
        sqlfile.write('INSERT INTO clinched VALUES\n')
        first = True
        for c in clinched_list[start:start+10000]:
            if not first:
                sqlfile.write(",")
            first = False
            sqlfile.write("(" + c + ")\n")
        sqlfile.write(";\n")
        
    # overall mileage by region data (with concurrencies accounted for,
    # active systems only then active+preview)
    sqlfile.write('CREATE TABLE overallMileageByRegion (region VARCHAR(8), activeMileage FLOAT, activePreviewMileage FLOAT);\n')
    sqlfile.write('INSERT INTO overallMileageByRegion VALUES\n')
    first = True
    for region in list(active_preview_mileage_by_region.keys()):
        if not first:
            sqlfile.write(",")
        first = False
        active_only_mileage = 0.0
        active_preview_mileage = 0.0
        if region in list(active_only_mileage_by_region.keys()):
            active_only_mileage = active_only_mileage_by_region[region]
        if region in list(active_preview_mileage_by_region.keys()):
            active_preview_mileage = active_preview_mileage_by_region[region]
        sqlfile.write("('" + region + "','" +
                      str(active_only_mileage) + "','" +
                      str(active_preview_mileage) + "')\n")
    sqlfile.write(";\n")

    # system mileage by region data (with concurrencies accounted for,
    # active systems and preview systems only)
    sqlfile.write('CREATE TABLE systemMileageByRegion (systemName VARCHAR(10), region VARCHAR(8), mileage FLOAT, FOREIGN KEY (systemName) REFERENCES systems(systemName));\n')
    sqlfile.write('INSERT INTO systemMileageByRegion VALUES\n')
    first = True
    for h in highway_systems:
        if h.active_or_preview():
            for region in list(h.mileage_by_region.keys()):
                if not first:
                    sqlfile.write(",")
                first = False
                sqlfile.write("('" + h.systemname + "','" + region + "','" + str(h.mileage_by_region[region]) + "')\n")
    sqlfile.write(";\n")

    # clinched overall mileage by region data (with concurrencies
    # accounted for, active systems and preview systems only)
    sqlfile.write('CREATE TABLE clinchedOverallMileageByRegion (region VARCHAR(8), traveler VARCHAR(48), activeMileage FLOAT, activePreviewMileage FLOAT);\n')
    sqlfile.write('INSERT INTO clinchedOverallMileageByRegion VALUES\n')
    first = True
    for t in traveler_lists:
        for region in list(t.active_preview_mileage_by_region.keys()):
            if not first:
                sqlfile.write(",")
            first = False
            active_miles = 0.0
            if region in list(t.active_only_mileage_by_region.keys()):
                active_miles = t.active_only_mileage_by_region[region]
            sqlfile.write("('" + region + "','" + t.traveler_name + "','" +
                          str(active_miles) + "','" +
                          str(t.active_preview_mileage_by_region[region]) + "')\n")
    sqlfile.write(";\n")

    # clinched system mileage by region data (with concurrencies accounted
    # for, active systems and preview systems only)
    sqlfile.write('CREATE TABLE clinchedSystemMileageByRegion (systemName VARCHAR(10), region VARCHAR(8), traveler VARCHAR(48), mileage FLOAT, FOREIGN KEY (systemName) REFERENCES systems(systemName));\n')
    sqlfile.write('INSERT INTO clinchedSystemMileageByRegion VALUES\n')
    first = True
    for line in csmbr_values:
        if not first:
            sqlfile.write(",")
        first = False
        sqlfile.write(line + "\n")
    sqlfile.write(";\n")

    # clinched mileage by connected route, active systems and preview
    # systems only
    sqlfile.write('CREATE TABLE clinchedConnectedRoutes (route VARCHAR(32), traveler VARCHAR(48), mileage FLOAT, clinched BOOLEAN, FOREIGN KEY (route) REFERENCES connectedRoutes(firstRoot));\n')
    for start in range(0, len(ccr_values), 10000):
        sqlfile.write('INSERT INTO clinchedConnectedRoutes VALUES\n')
        first = True
        for line in ccr_values[start:start+10000]:
            if not first:
                sqlfile.write(",")
            first = False
            sqlfile.write(line + "\n")
        sqlfile.write(";\n")

    # clinched mileage by route, active systems and preview systems only
    sqlfile.write('CREATE TABLE clinchedRoutes (route VARCHAR(32), traveler VARCHAR(48), mileage FLOAT, clinched BOOLEAN, FOREIGN KEY (route) REFERENCES routes(root));\n')
    for start in range(0, len(cr_values), 10000):
        sqlfile.write('INSERT INTO clinchedRoutes VALUES\n')
        first = True
        for line in cr_values[start:start+10000]:
            if not first:
                sqlfile.write(",")
            first = False
            sqlfile.write(line + "\n")
        sqlfile.write(";\n")

    # updates entries
    sqlfile.write('CREATE TABLE updates (date VARCHAR(10), region VARCHAR(60), route VARCHAR(80), root VARCHAR(32), description VARCHAR(1024));\n')
    sqlfile.write('INSERT INTO updates VALUES\n')
    first = True
    for update in updates:
        if not first:
            sqlfile.write(",")
        first = False
        sqlfile.write("('"+update[0]+"','"+update[1].replace("'","''")+"','"+update[2].replace("'","''")+"','"+update[3]+"','"+update[4].replace("'","''")+"')\n")
    sqlfile.write(";\n")

    # systemUpdates entries
    sqlfile.write('CREATE TABLE systemUpdates (date VARCHAR(10), region VARCHAR(48), systemName VARCHAR(10), description VARCHAR(128), statusChange VARCHAR(16));\n')
    sqlfile.write('INSERT INTO systemUpdates VALUES\n')
    first = True
    for systemupdate in systemupdates:
        if not first:
            sqlfile.write(",")
        first = False
        sqlfile.write("('"+systemupdate[0]+"','"+systemupdate[1].replace("'","''")+"','"+systemupdate[2]+"','"+systemupdate[3].replace("'","''")+"','"+systemupdate[4]+"')\n")
    sqlfile.write(";\n")

    # datacheck errors into the db
    sqlfile.write('CREATE TABLE datacheckErrors (route VARCHAR(32), label1 VARCHAR(50), label2 VARCHAR(20), label3 VARCHAR(20), code VARCHAR(20), value VARCHAR(32), falsePositive BOOLEAN, FOREIGN KEY (route) REFERENCES routes(root));\n')
    if len(datacheckerrors) > 0:
        sqlfile.write('INSERT INTO datacheckErrors VALUES\n')
        first = True
        for d in datacheckerrors:
            if not first:
                sqlfile.write(',')
            first = False
            sqlfile.write("('"+str(d.route.root)+"',")
            if len(d.labels) == 0:
                sqlfile.write("'','','',")
            elif len(d.labels) == 1:
                sqlfile.write("'"+d.labels[0]+"','','',")
            elif len(d.labels) == 2:
                sqlfile.write("'"+d.labels[0]+"','"+d.labels[1]+"','',")
            else:
                sqlfile.write("'"+d.labels[0]+"','"+d.labels[1]+"','"+d.labels[2]+"',")
            if d.fp:
                fp = '1'
            else:
                fp = '0'
            sqlfile.write("'"+d.code+"','"+d.info+"','"+fp+"')\n")
    sqlfile.write(";\n")

    # update graph info in DB if graphs were generated
    if not args.skipgraphs:
        sqlfile.write('DROP TABLE IF EXISTS graphs;\n')
        sqlfile.write('DROP TABLE IF EXISTS graphTypes;\n')
        sqlfile.write('CREATE TABLE graphTypes (category VARCHAR(12), descr VARCHAR(100), longDescr TEXT, PRIMARY KEY(category));\n')
        if len(graph_types) > 0:
            sqlfile.write('INSERT INTO graphTypes VALUES\n')
            first = True
            for g in graph_types:
                if not first:
                    sqlfile.write(',')
                first = False
                sqlfile.write("('" + g[0] + "','" + g[1] + "','" + g[2] + "')\n")
            sqlfile.write(";\n")

        sqlfile.write('CREATE TABLE graphs (filename VARCHAR(32), descr VARCHAR(100), vertices INTEGER, edges INTEGER, format VARCHAR(10), category VARCHAR(12), FOREIGN KEY (category) REFERENCES graphTypes(category));\n')
        if len(graph_list) > 0:
            sqlfile.write('INSERT INTO graphs VALUES\n')
            first = True
            for g in graph_list:
                if not first:
                    sqlfile.write(',')
                first = False
                sqlfile.write("('" + g.filename + "','" + g.descr.replace("'","''") + "','" + str(g.vertices) + "','" + str(g.edges) + "','" + g.format + "','" + g.category + "')\n")
            sqlfile.write(";\n")


    sqlfile.close()

# print some statistics
print(et.et() + "Processed " + str(len(highway_systems)) + " highway systems.")
routes = 0
points = 0
segments = 0
for h in highway_systems:
    routes += len(h.route_list)
    for r in h.route_list:
        points += len(r.point_list)
        segments += len(r.segment_list)
print("Processed " + str(routes) + " routes with a total of " + \
          str(points) + " points and " + str(segments) + " segments.")
if points != all_waypoints.size():
    print("MISMATCH: all_waypoints contains " + str(all_waypoints.size()) + " waypoints!")
print("WaypointQuadtree contains " + str(all_waypoints.total_nodes()) + " total nodes.")

if not args.errorcheck:
    # compute colocation of waypoints stats
    print(et.et() + "Computing waypoint colocation stats, reporting all with 8 or more colocations:")
    largest_colocate_count = all_waypoints.max_colocated()
    colocate_counts = [0]*(largest_colocate_count+1)
    big_colocate_locations = dict()
    for w in all_waypoints.point_list():
        c = w.num_colocated()
        if c >= 8:
            point = (w.lat, w.lng)
            entry = w.route.root + " " + w.label
            if point in big_colocate_locations:
                the_list = big_colocate_locations[point]
                the_list.append(entry)
                big_colocate_locations[point] = the_list
            else:
                the_list = []
                the_list.append(entry)
                big_colocate_locations[point] = the_list
            #print(str(w) + " with " + str(c) + " other points.")
        colocate_counts[c] += 1
    for place in big_colocate_locations:
        the_list = big_colocate_locations[place]
        print(str(place) + " is occupied by " + str(len(the_list)) + " waypoints: " + str(the_list))
    print("Waypoint colocation counts:")
    unique_locations = 0
    for c in range(1,largest_colocate_count+1):
        unique_locations += colocate_counts[c]//c
        print("{0:6d} are each occupied by {1:2d} waypoints.".format(colocate_counts[c]//c, c))
    print("Unique locations: " + str(unique_locations))

if args.errorcheck:
    print("!!! DATA CHECK SUCCESSFUL !!!")

print("Total run time: " + et.et())
