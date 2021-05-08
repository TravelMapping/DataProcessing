#!/usr/bin/env python3
# Travel Mapping Project, Jim Teresco, 2015-2021
"""Python code to read .csv and .wpt files and prepare for
adding to the Travel Mapping Project database.

(c) 2015-2021, Jim Teresco, Eric Bryant, and Travel Mapping Project contributors

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
import string
import time
import threading

class DBFieldLength:
    abbrev = 3
    banner = 6
    city = 100
    color = 16
    continentCode = 3
    continentName = 15
    countryCode = 3
    countryName = 32
    date = 10
    dcErrCode = 22
    graphCategory = 12
    graphDescr = 100
    graphFilename = 32
    graphFormat = 10
    label = 26
    level = 10
    regionCode = 8
    regionName = 48
    regiontype = 32
    root = 32
    route = 16
    routeLongName = 80
    statusChange = 16
    systemFullName = 60
    systemName = 10
    traveler = 48
    updateText = 1024

    countryRegion = countryName + regionName + 3
    dcErrValue = root + label + 1

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
            self.insert(p, False)

    def insert(self,w,init):
        """insert Waypoint w into this quadtree node"""
        #print("QTDEBUG: " + str(self) + " insert " + str(w))
        if self.points is not None:
            # look for colocated points during initial insertion
            if init:
                other_w = None
                for p in self.points:
                    if p.same_coords(w):
                        other_w = p
                        break
                if other_w is not None:
                    # see if this is the first point colocated with other_w
                    if other_w.colocated is None:
                        other_w.colocated = [ other_w ]
                    other_w.colocated.append(w)
                    w.colocated = other_w.colocated
            if w.colocated is None or w == w.colocated[0]:
                #print("QTDEBUG: " + str(self) + " at " + str(self.unique_locations) + " unique locations")
                self.unique_locations += 1
            self.points.append(w)
            if self.unique_locations > 50:  # 50 unique points max per quadtree node
                self.refine()
        else:
            if w.lat < self.mid_lat:
                if w.lng < self.mid_lng:
                    self.sw_child.insert(w, init)
                else:
                    self.se_child.insert(w, init)
            else:
                if w.lng < self.mid_lng:
                    self.nw_child.insert(w, init)
                else:
                    self.ne_child.insert(w, init)

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

    def graph_points(self, hi_priority_points, lo_priority_points):
        # return a list of points to be used as indices to HighwayGraph vertices
        if self.points is None:
            self.ne_child.graph_points(hi_priority_points, lo_priority_points)
            self.nw_child.graph_points(hi_priority_points, lo_priority_points)
            self.se_child.graph_points(hi_priority_points, lo_priority_points)
            self.sw_child.graph_points(hi_priority_points, lo_priority_points)
        else:
            for w in self.points:
                # skip if this point is occupied by only waypoints in devel systems
                if not w.is_or_colocated_with_active_or_preview():
                    continue
                # skip if colocated and not at front of list
                if  w.colocated is not None and w != w.colocated[0]:
                    continue
                # store a colocated list with any devel system entries removed
                if w.colocated is None:
                    w.ap_coloc = [w]
                else:
                    w.ap_coloc = []
                    for p in w.colocated:
                        if p.route.system.active_or_preview():
                            w.ap_coloc.append(p)
                # determine vertex name simplification priority
                if len(w.ap_coloc) != 2 \
                or len(w.ap_coloc[0].route.abbrev) > 0 \
                or len(w.ap_coloc[1].route.abbrev) > 0:
                    lo_priority_points.append(w)
                else:
                    hi_priority_points.append(w)

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
            for w in self.points:
                if w.colocated:
                    w.colocated.sort(key=lambda waypoint: waypoint.route.root + "@" + waypoint.label)

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
            datacheckerrors.append(DatacheckEntry(route,[self.label],'MALFORMED_URL', 'MISSING_ARG(S)'))
            self.lat = 0.0
            self.lng = 0.0
            self.colocated = None
            self.near_miss_points = None
            return
        lat_string = url_parts[1].split("&")[0] # chop off "&lon"
        lng_string = url_parts[2].split("&")[0] # chop off possible "&zoom"
        valid_coords = True
        if not valid_num_str(lat_string):
            if len(lat_string.encode('utf-8')) > DBFieldLength.dcErrValue:
                slicepoint = DBFieldLength.dcErrValue-3
                while len(lat_string[:slicepoint].encode('utf-8')) > DBFieldLength.dcErrValue-3:
                        slicepoint -= 1
                lat_string = lat_string[:slicepoint]+"..."
            datacheckerrors.append(DatacheckEntry(route, [self.label], "MALFORMED_LAT", lat_string))
            valid_coords = False
        if not valid_num_str(lng_string):
            if len(lng_string.encode('utf-8')) > DBFieldLength.dcErrValue:
                slicepoint = DBFieldLength.dcErrValue-3
                while len(lng_string[:slicepoint].encode('utf-8')) > DBFieldLength.dcErrValue-3:
                        slicepoint -= 1
                lng_string = lng_string[:slicepoint]+"..."
            datacheckerrors.append(DatacheckEntry(route, [self.label], "MALFORMED_LON", lng_string))
            valid_coords = False
        if valid_coords:
            self.lat = float(lat_string)
            self.lng = float(lng_string)
        else:
            self.lat = 0
            self.lng = 0
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

        # original formula
        """ans = math.acos(math.cos(rlat1)*math.cos(rlng1)*math.cos(rlat2)*math.cos(rlng2) +\
                        math.cos(rlat1)*math.sin(rlng1)*math.cos(rlat2)*math.sin(rlng2) +\
                        math.sin(rlat1)*math.sin(rlat2)) * 3963.1 # EARTH_RADIUS """

        # spherical law of cosines formula (same as orig, with some terms factored out or removed via trig identity)
        """ans = math.acos(math.cos(rlat1)*math.cos(rlat2)*math.cos(rlng2-rlng1)+math.sin(rlat1)*math.sin(rlat2)) * 3963.1 # EARTH_RADIUS """

        # Vincenty formula
        """ans = math.atan\
        (   math.sqrt(pow(math.cos(rlat2)*math.sin(rlng2-rlng1),2)+pow(math.cos(rlat1)*math.sin(rlat2)-math.sin(rlat1)*math.cos(rlat2)*math.cos(rlng2-rlng1),2))
            /
            (math.sin(rlat1)*math.sin(rlat2)+math.cos(rlat1)*math.cos(rlat2)*math.cos(rlng2-rlng1))
        ) * 3963.1 # EARTH_RADIUS """

        # haversine formula
        ans = math.asin(math.sqrt(pow(math.sin((rlat2-rlat1)/2),2) + math.cos(rlat1) * math.cos(rlat2) * pow(math.sin((rlng2-rlng1)/2),2))) * 7926.2 # EARTH_DIAMETER """

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

    def canonical_waypoint_name(self,log, vertex_names, datacheckerrors):
        """Best name we can come up with for this point bringing in
        information from itself and colocated points (if active/preview)
        """
        # start with the failsafe name, and see if we can improve before
        # returning
        name = self.simple_waypoint_name()

        # if no colocated active/preview points, there's nothing to do - we
        # just return the simple name and deal with conflicts elsewhere
        if (len(self.ap_coloc) == 1):
            return name

        # straightforward 2-route intersection with matching labels
        # NY30@US20&US20@NY30 would become NY30/US20
        # or
        # 2-route intersection with one or both labels having directional
        # suffixes but otherwise matching route
        # US24@CO21_N&CO21@US24_E would become US24_E/CO21_N

        if len(self.ap_coloc) == 2:
            # check both refs independently, because datachecks are involved
            one_ref_zero = self.ap_coloc[1].label_references_route(self.ap_coloc[0].route, datacheckerrors)
            zero_ref_one = self.ap_coloc[0].label_references_route(self.ap_coloc[1].route, datacheckerrors)
            if one_ref_zero and zero_ref_one:
                newname = self.ap_coloc[1].label+"/"+self.ap_coloc[0].label
                # if this is taken or if name_no_abbrev()s match, attempt to add in abbrevs if there's point in doing so
                if len(self.ap_coloc[0].route.abbrev) > 0 or len(self.ap_coloc[1].route.abbrev) > 0:
                    taken = newname in vertex_names
                    if taken or self.ap_coloc[0].route.name_no_abbrev() == self.ap_coloc[1].route.name_no_abbrev():
                        newname = self.ap_coloc[0].route.list_entry_name() \
                                + (self.ap_coloc[1].label[self.ap_coloc[1].label.index('_'):] if '_' in self.ap_coloc[1].label else "") \
                                + "/" \
                                + self.ap_coloc[1].route.list_entry_name() \
                                + (self.ap_coloc[0].label[self.ap_coloc[0].label.index('_'):] if '_' in self.ap_coloc[0].label else "")
                        message = "Straightforward_intersection: " + name + " -> " + newname
                        if taken:
                            message += " (" + self.ap_coloc[1].label+"/"+self.ap_coloc[0].label + " already taken)"
                        log.append(message)
                        return newname
                log.append("Straightforward_intersection: " + name + " -> " + newname)
                return newname

        # straightforward concurrency example with matching waypoint
        # labels, use route/route/route@label, except also matches
        # any hidden label
        # TODO: compress when some but not all labels match, such as
        # E127@Kan&AH60@Kan_N&AH64@Kan&AH67@Kan&M38@Kan
        # or possibly just compress ignoring the _ suffixes here
        routes = []
        matches = 0
        for w in self.ap_coloc:
            if self.ap_coloc[0].label == w.label or w.label[0] == '+':
                # avoid double route names at border crossings
                if w.route.list_entry_name() not in routes:
                    routes.append(w.route.list_entry_name())
                matches += 1
            else:
                break
        if matches == len(self.ap_coloc):
            newname = ""
            for r in routes:
                newname += '/' + r
            newname = newname[1:]
            newname += '@' + self.ap_coloc[0].label
            log.append("Straightforward_concurrency: " + name + " -> " + newname)
            return newname

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

        for match_index in range(0,len(self.ap_coloc)):
            lookfor1 = self.ap_coloc[match_index].route.list_entry_name()
            lookfor2 = self.ap_coloc[match_index].route.list_entry_name() + \
               '(' + self.ap_coloc[match_index].label + ')'
            all_match = True
            for check_index in range(0,len(self.ap_coloc)):
                if match_index == check_index:
                    continue
                if (self.ap_coloc[check_index].label != lookfor1) and \
                   (self.ap_coloc[check_index].label != lookfor2):
                    all_match = False
                    break
            if all_match:
                if (self.ap_coloc[match_index].label[0:1].isnumeric()):
                    label = lookfor2
                else:
                    label = lookfor1
                for add_index in range(0,len(self.ap_coloc)):
                    if match_index == add_index:
                        continue
                    label += '/' + self.ap_coloc[add_index].route.list_entry_name()
                log.append("Exit/Intersection: " + name + " -> " + label)
                return label

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
        if len(self.ap_coloc) > 2:
            suffixes = [""] * len(self.ap_coloc)
            for check in self.ap_coloc:
                match = False
                for index, other in enumerate(self.ap_coloc):
                    if other == check:
                        continue
                    other_no_abbrev = other.route.name_no_abbrev()
                    if check.label.startswith(other_no_abbrev):
                        # should check here for false matches, like
                        # NY50/67 would match startswith NY5
                        match = True
                        if '_' in check.label:
                            suffix = check.label[check.label.find('_'):]
                            # we confirmed above that other_no_abbrev matches, so skip past it
                            # we need only match the suffix with or without the abbrev
                            if check.label[len(other_no_abbrev):] == suffix \
                            or other.route.abbrev + suffix == check.label[len(other_no_abbrev):]:
                                suffixes[index] = suffix
                if not match:
                    break
            if match:
                label = self.ap_coloc[0].route.list_entry_name() + suffixes[0]
                for index in range(1,len(self.ap_coloc)):
                    label += "/" + self.ap_coloc[index].route.list_entry_name() + suffixes[index]
                log.append("3+_intersection: " + name + " -> " + label)
                return label

        # Exit number simplification: I-90@47B(94)&I-94@47B
        # becomes I-90/I-94@47B, with many other cases also matched
        # Still TODO: I-39@171C(90)&I-90@171C&US14@I-39/90
        # try each as a possible route@exit type situation and look
        # for matches
        for exit in self.ap_coloc:
            # see if all colocated points are potential matches
            # when considering the one at exit as a primary
            # exit number
            if not exit.label[0].isdigit():
                continue
            list_name = exit.route.list_entry_name()
            no_abbrev = exit.route.name_no_abbrev()
            nmbr_only = no_abbrev # route number only version
            for pos in range(len(nmbr_only)):
                if nmbr_only[pos].isdigit():
                    nmbr_only = nmbr_only[pos:]
                    break
            all_match = True
            for match in self.ap_coloc:
                if exit == match:
                    continue
                # check for any of the patterns that make sense as a match:
                # exact match
                # match concurrency exit number format nn(rr)
                # match with exit number only

                # if label_no_abbrev() matches, check for...
                if match.label == no_abbrev:
                    continue		# full match without abbrev field
                if match.label.startswith(no_abbrev):
                    if match.label[len(no_abbrev)] in '_/':
                        continue	# match with _ suffix (like _N) or slash
                    if match.label[len(no_abbrev)] == '(' \
                    and match.label[len(no_abbrev)+1:len(no_abbrev)+len(exit.label)+1] == exit.label \
                    and match.label[len(no_abbrev)+len(exit.label)+1] == ')':
                        continue	# match with exit number in parens

                if (    match.label != list_name
                    and match.label != list_name + "(" + exit.label + ")"
                    and match.label != exit.label
                    and match.label != exit.label + "(" + nmbr_only + ")"
                    and match.label != exit.label + "(" + no_abbrev + ")"):
                    all_match = False
                    break
            if all_match:
                label = ""
                for pos in range(len(self.ap_coloc)):
                    label += self.ap_coloc[pos].route.list_entry_name()
                    if self.ap_coloc[pos] == exit:
                        label += "(" + self.ap_coloc[pos].label + ")"
                    if pos < len(self.ap_coloc) - 1:
                        label += "/"
                log.append("Exit_number: " + name + " -> " + label)
                return label

        # Check for reversed border labels
        # DE491@DE/PA&PA491@PA/DE                                             would become   DE491/PA491@PA/DE
        # NB2@QC/NB&TCHMai@QC/NB&A-85Not@NB/QC&TCHMai@QC/NB                   would become   NB2/TCHMai/A-85Not/@QC/NB
        # US12@IL/IN&US20@IL/IN&US41@IN/IL&US12@IL/IN&US20@IL/IN&US41@IN/IL   would become   US12/US20/US41@IL/IN
        if '/' in self.label:
            slash = self.label.index('/')
            reverse = self.label[slash+1:]+'/'+self.label[:slash]
            i = 1
            # self.ap_coloc[0].label *IS* label, so no need to check that
            while i < len(self.ap_coloc):
              if self.ap_coloc[i].label == self.label or self.ap_coloc[i].label == reverse:
                i += 1
              else:
                  break
            if i == len(self.ap_coloc):
                routes = []
                for w in self.ap_coloc:
                    if w.route.list_entry_name() not in routes:
                        routes.append(w.route.list_entry_name())

                newname = routes[0]
                for i in range(1, len(routes)):
                    newname += '/' + routes[i]
                newname += '@' + self.label
                log.append("Reversed_border_labels: " + name + " -> " + newname)
                return newname

        # TODO: I-20@76&I-77@16
        # should become I-20/I-77 or maybe I-20(76)/I-77(16)
        # not shorter, so maybe who cares about this one?

        # TODO: US83@FM1263_S&US380@FM1263
        # should probably end up as US83/US380@FM1263 or @FM1263_S

        # How about?
        # I-581@4&US220@I-581(4)&US460@I-581&US11AltRoa@I-581&US220AltRoa@US220_S&VA116@I-581(4)

        # TODO: I-610@TX288&I-610@38&TX288@I-610
        # this is the overlap point of a loop

        log.append("Keep_failsafe: " + name)
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

    def hashpoint(self):
        # return a canonical waypoint for graph vertex hashtable lookup
        if self.colocated is None:
            return self
        return self.colocated[0]

    def label_references_route(self, r, datacheckerrors):
        no_abbrev = r.name_no_abbrev()
        if self.label[:len(no_abbrev)] != no_abbrev:
            return False
        if len(self.label) == len(no_abbrev) \
        or self.label[len(no_abbrev)] == '_':
            return True
        if self.label[len(no_abbrev):len(no_abbrev)+len(r.abbrev)] != r.abbrev:
            #if self.label[len(no_abbrev)] == '/':
                #datacheckerrors.append(DatacheckEntry(route, [self.label], "UNEXPECTED_DESIGNATION", self.label[len(no_abbrev)+1:]))
            return False
        if len(self.label) == len(no_abbrev) + len(r.abbrev) \
        or self.label[len(no_abbrev) + len(r.abbrev)] == '_':
            return True
        #if self.label[len(no_abbrev) + len(r.abbrev)] == '/':
            #datacheckerrors.append(DatacheckEntry(route, [self.label], "UNEXPECTED_DESIGNATION", self.label[len(no_abbrev)+len(r.abbrev)+1:]))
        return False

    def label_too_long(self, datacheckerrors):
        # check whether label is longer than the DB can store
        if len(self.label.encode('utf-8')) > DBFieldLength.label:
            slicepoint = DBFieldLength.label-3
            while len(self.label[:slicepoint].encode('utf-8')) > DBFieldLength.label-3:
                slicepoint -= 1
            excess = "..."+self.label[slicepoint:]
            self.label = self.label[:slicepoint]

            if len(excess.encode('utf-8')) > DBFieldLength.dcErrValue:
                slicepoint = DBFieldLength.dcErrValue-3
                while len(excess[:slicepoint].encode('utf-8')) > DBFieldLength.dcErrValue-3:
                        slicepoint -= 1
                excess = excess[:slicepoint]+"..."
            datacheckerrors.append(DatacheckEntry(self.route,[self.label+'...'],"LABEL_TOO_LONG", excess))
            return True
        return False

class HighwaySegment:
    """This class represents one highway segment: the connection between two
    Waypoints connected by one or more routes"""

    def __init__(self,w1,w2,route):
        self.waypoint1 = w1
        self.waypoint2 = w2
        self.route = route
        self.length = self.waypoint1.distance_to(self.waypoint2)
        self.concurrent = None
        self.clinched_by = set()

    def __str__(self):
        return self.route.readable_name() + " " + self.waypoint1.label + " " + self.waypoint2.label

    def add_clinched_by(self,traveler):
        if traveler not in self.clinched_by:
            self.clinched_by.add(traveler)
            return True
        else:
            return False

    def csv_line(self,id):
        """return csv line to insert into a table"""
        return "'" + str(id) + "','" + str(self.waypoint1.point_num) + "','" + str(self.waypoint2.point_num) + "','" + self.route.root + "'"

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

    def concurrent_travelers_sanity_check(self):
        if self.route.system.devel():
            return ""
        if self.concurrent is not None:
            for conc in self.concurrent:
                if len(self.clinched_by) != len(conc.clinched_by):
                    if conc.route.system.devel():
                        return ""
                    return '[' + str(self) + ']' + " clinched by " + str(len(self.clinched_by)) + " travelers; " \
                         + '[' + str(conc) + ']' + " clinched by " + str(len(conc.clinched_by)) + '\n'
                else:
                    for t in self.clinched_by:
                        if t not in conc.clinched_by:
                            return t.traveler_name + " has clinched [" + str(self) + "], but not [" + str(conc) + "]\n"
        return ""

    def clinchedby_code(self, traveler_lists):
        # Return a hexadecimal string encoding which travelers have clinched this segment, for use in "traveled" graph files
        # Each character stores info for traveler #n thru traveler #n+3
        # The 1st character stores traveler 0 thru traveler 3,
        # The 2nd character stores traveler 4 thru traveler 7, etc.
        # For each character, the low-order bit stores traveler n, and the high bit traveler n+3.

        if len(traveler_lists) == 0:
            return "0"
        code = ""
        clinch_array = [0]*( math.ceil(len(traveler_lists)/4) )
        for t in self.clinched_by:
            clinch_array[t.traveler_num // 4] += 2 ** (t.traveler_num % 4)
        for c in clinch_array:
            code += "0123456789ABCDEF"[c]
        return code

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
    root_hash = dict()
    pri_list_hash = dict()
    alt_list_hash = dict()

    def __init__(self,line,system,el):
        """initialize object from a .csv file line, but do not
        yet read in waypoint file"""
        self.root = ""
        fields = line.split(";")
        if len(fields) != 8:
            el.add_error("Could not parse " + system.systemname + ".csv line: [" + line +
                         "], expected 8 fields, found " + str(len(fields)))
            return
        self.system = system
        if system.systemname != fields[0]:
            el.add_error("System mismatch parsing " + system.systemname +
                         ".csv line [" + line + "], expected " + system.systemname)
        self.region = fields[1]
        region_found = False
        if self.region not in all_regions:
            el.add_error("Unrecognized region in " + system.systemname +
                         ".csv line: " + line)
        self.route = fields[2]
        if len(self.route.encode('utf-8')) > DBFieldLength.route:
            el.add_error("Route > " + str(DBFieldLength.route) +
                         " bytes in " + system.systemname +
                         ".csv line: " + line)
        self.banner = fields[3]
        if len(self.banner.encode('utf-8')) > DBFieldLength.banner:
            el.add_error("Banner > " + str(DBFieldLength.banner) +
                         " bytes in " + system.systemname +
                         ".csv line: " + line)
        self.abbrev = fields[4]
        if len(self.abbrev.encode('utf-8')) > DBFieldLength.abbrev:
            el.add_error("Abbrev > " + str(DBFieldLength.abbrev) +
                         " bytes in " + system.systemname +
                         ".csv line: " + line)
        self.city = fields[5]
        if len(self.city.encode('utf-8')) > DBFieldLength.city:
            el.add_error("City > " + str(DBFieldLength.city) +
                         " bytes in " + system.systemname +
                         ".csv line: " + line)
        self.root = fields[6].lower()
        if len(self.root.encode('utf-8')) > DBFieldLength.root:
            el.add_error("Root > " + str(DBFieldLength.root) +
                         " bytes in " + system.systemname +
                         ".csv line: " + line)
        if len(fields[7]) == 0:
            self.alt_route_names = []
        else:
            self.alt_route_names = fields[7].upper().split(",")

        # insert into root_hash, checking for duplicate root entries
        if self.root in Route.root_hash:
            el.add_error("Duplicate root in " + system.systemname + ".csv: " + self.root +
                         " already in " + Route.root_hash[self.root].system.systemname + ".csv")
        else:
            Route.root_hash[self.root] = self
        # insert list name into pri_list_hash, checking for duplicate .list names
        list_name = self.readable_name().upper()
        if list_name in Route.alt_list_hash:
            el.add_error("Duplicate main list name in " + self.root + ": '" + self.readable_name() +
                         "' already points to " + Route.alt_list_hash[list_name].root)
        elif list_name in Route.pri_list_hash:
            el.add_error("Duplicate main list name in " + self.root + ": '" + self.readable_name() +
                         "' already points to " + Route.pri_list_hash[list_name].root)
        else:
            Route.pri_list_hash[list_name] = self
        # insert alt names into alt_list_hash, checking for duplicate .list names
        for a in self.alt_route_names:
            list_name = self.region.upper() + ' ' + a
            if list_name in Route.pri_list_hash:
                el.add_error("Duplicate alt route name in " + self.root + ": '" + self.region + ' ' + a +
                             "' already points to " + Route.pri_list_hash[list_name].root)
            elif list_name in Route.alt_list_hash:
                el.add_error("Duplicate alt route name in " + self.root + ": '" + self.region + ' ' + a +
                             "' already points to " + Route.alt_list_hash[list_name].root)
            else:
                Route.alt_list_hash[list_name] = self
            # populate unused set
            system.unusedaltroutenames.add(list_name)

        self.point_list = []
        self.labels_in_use = set()
        self.unused_alt_labels = set()
        self.duplicate_labels = set()
        self.pri_label_hash = dict()
        self.alt_label_hash = dict()
        self.segment_list = []
        self.con_route = None
        self.last_update = None
        self.mileage = 0.0
        self.rootOrder = -1  # order within connected route
        self.is_reversed = False

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
                    malformed_url = w.lat == 0.0 and w.lng == 0.0
                    label_too_long = w.label_too_long(datacheckerrors)
                    if malformed_url or label_too_long:
                        w = previous_point
                        continue
                    self.point_list.append(w)

                    all_waypoints_lock.acquire()
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
                        w.near_miss_points = nmps
                        for other_w in nmps:
                            if other_w.near_miss_points is None:
                                other_w.near_miss_points = [ w ]
                            else:
                                other_w.near_miss_points.append(w)
    
                    all_waypoints.insert(w, True)
                    all_waypoints_lock.release()
                    # add HighwaySegment, if not first point
                    if previous_point is not None:
                        self.segment_list.append(HighwaySegment(previous_point, w, self))
            if len(self.point_list) < 2:
                el.add_error("Route contains fewer than 2 points: " + str(self))
            print(".", end="",flush=True)

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
        return "'" + self.system.systemname + "','" + self.region + "','" + self.route + "','" + self.banner + "','" + self.abbrev + \
               "','" + self.city.replace("'","''") + "','" + self.root + "','" + str(self.mileage) + "','" + str(self.rootOrder) + "'";

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
                miles += s.length
        return miles

    def store_traveled_segments(self, t, beg, end):
        # store clinched segments with traveler and traveler with segments
        for pos in range(beg, end):
            hs = self.segment_list[pos]
            hs.add_clinched_by(t)
            t.clinched_segments.add(hs)
        if self not in t.routes:
            t.routes.add(self)
            if self.last_update and t.update and self.last_update[0] >= t.update:
                t.log_entries.append("Route updated " + self.last_update[0] + ": " + self.readable_name())

    def con_beg(self):
        return self.point_list[-1] if self.is_reversed else self.point_list[0]

    def con_end(self):
        return self.point_list[0] if self.is_reversed else self.point_list[-1]

class ConnectedRoute:
    """This class encapsulates a single 'connected route' as given
    by a single line of a _con.csv file
    """

    def __init__(self,line,system,el):
        """initialize the object from the _con.csv line given"""
        self.system = system
        self.route = ""
        self.banner = ""
        self.groupname = ""
        self.roots = []
        self.mileage = 0.0
        fields = line.split(";")
        if len(fields) != 5:
            el.add_error("Could not parse " + system.systemname + "_con.csv line: [" + line +
                         "], expected 5 fields, found " + str(len(fields)))
            return
        if system.systemname != fields[0]:
            el.add_error("System mismatch parsing " + system.systemname +
                         "_con.csv line [" + line + "], expected " + system.systemname)
        self.route = fields[1]
        if len(self.route.encode('utf-8')) > DBFieldLength.route:
            el.add_error("route > " + str(DBFieldLength.route) +
                         " bytes in " + system.systemname +
                         "_con.csv line: " + line)
        self.banner = fields[2]
        if len(self.banner.encode('utf-8')) > DBFieldLength.banner:
            el.add_error("banner > " + str(DBFieldLength.banner) +
                         " bytes in " + system.systemname +
                         "_con.csv line: " + line)
        self.groupname = fields[3]
        if len(self.groupname.encode('utf-8')) > DBFieldLength.city:
            el.add_error("groupname > " + str(DBFieldLength.city) +
                         " bytes in " + system.systemname +
                         "_con.csv line: " + line)
        # fields[4] is the list of roots, which will become a python list
        # of Route objects already in the system
        rootOrder = 0
        for root in fields[4].lower().split(","):
            try:
                route = Route.root_hash[root]
                self.roots.append(route)
                if route.con_route is not None:
                    el.add_error("Duplicate root in " + system.systemname + "_con.csv: " + route.root +
                                 " already in " + route.con_route.system.systemname + "_con.csv")
                if system != route.system:
                    el.add_error("System mismatch: chopped route " + route.root + " from " + route.system.systemname +
                        ".csv in connected route in " + system.systemname + "_con.csv");
                route.con_route = self
                # save order of route in connected route
                route.rootOrder = rootOrder
                rootOrder += 1
            except KeyError:
                el.add_error("Could not find Route matching ConnectedRoute root " + root +
                             " in system " + system.systemname + '.')
        if len(self.roots) < 1:
            el.add_error("No roots in " + system.systemname + "_con.csv line: " + line)
        # will be computed for routes in active & preview systems later

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

    With the implementation of three levels of systems (active,
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
        self.vertices = set()
        self.listnamesinuse = set()
        self.unusedaltroutenames = set()

        # read chopped routes .csv
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
                line=line.strip()
                if len(line) > 0:
                    r = Route(line,self,el)
                    if r.root == "":
                        el.add_error("Unable to find root in " + systemname +
                                     ".csv line: ["+line+"]")
                    else:
                        self.route_list.append(r)

        # read _con.csv
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
                line=line.strip()
                if len(line) > 0:
                    self.con_route_list.append(ConnectedRoute(line,self,el))

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

    def __init__(self,travelername,update,el,path="../../../UserData/list_files"):
        list_entries = 0
        self.clinched_segments = set()
        self.traveler_name = travelername[:-5]
        if len(self.traveler_name.encode('utf-8')) > DBFieldLength.traveler:
            el.add_error("Traveler name " + self.traveler_name + " > " + str(DBFieldLength.traveler) + "bytes")
        with open(path+"/"+travelername,"rt", encoding='UTF-8') as file:
            lines = file.readlines()
        file.close()
        # strip UTF-8 byte order mark if present
        lines[0] = lines[0].encode('utf-8').decode("utf-8-sig")
        try:
            self.log_entries = [travelername+" last updated: "+update[0]+' '+update[1]+' '+update[2]]
            self.update = update[0]
        except TypeError:
            self.log_entries = []
            self.update = None
        self.routes = set()

        for line in lines:
            line = line.strip(" \t\r\n\x00")
            # ignore empty or "comment" lines
            if len(line) == 0 or line.startswith("#"):
                continue
            fields = re.split('[ \t]+',line)
            # truncate inline comments from fields list
            for i in range(min(len(fields), 7)):
                if fields[i][0] == '#':
                    fields = fields[:i]
                    break
            if len(fields) == 4:
                # find the route that matches and when we do, match labels
                lookup = fields[0].upper() + ' ' + fields[1].upper()
                # look for region/route combo, first in pri_list_hash
                try:
                    r = Route.pri_list_hash[lookup]
                # and then if not found, in alt_list_hash
                except KeyError:
                    try:
                        r = Route.alt_list_hash[lookup]
                    except KeyError:
                        (line, invchar) = no_control_chars(line)
                        self.log_entries.append("Unknown region/highway combo in line: " + line)
                        if invchar:
                            self.log_entries[-1] += " [contains invalid character(s)]"
                        continue
                    else:
                        self.log_entries.append("Note: deprecated route name " + fields[1] + " -> canonical name " + r.list_entry_name() + " in line: " + line)
                if r.system.devel():
                    self.log_entries.append("Ignoring line matching highway in system in development: " + line)
                    continue
                # r is a route match, and we need to find
                # waypoint indices, ignoring case and leading
                # "+" or "*" when matching
                list_label_1 = fields[2].lstrip("+*").upper()
                list_label_2 = fields[3].lstrip("+*").upper()

                # look for point indices for labels, first in pri_label_hash
                # and then if not found, in alt_label_hash
                try:
                    index1 = r.pri_label_hash[list_label_1]
                except KeyError:
                    try:
                        index1 = r.alt_label_hash[list_label_1]
                    except KeyError:
                        index1 = None
                try:
                    index2 = r.pri_label_hash[list_label_2]
                except KeyError:
                    try:
                        index2 = r.alt_label_hash[list_label_2]
                    except KeyError:
                        index2 = None
                # if we did not find matches for both labels...
                if index1 is None or index2 is None:
                    (list_label_1, invchar) = no_control_chars(list_label_1)
                    (list_label_2, invchar) = no_control_chars(list_label_2)
                    (line, invchar) = no_control_chars(line)
                    if index1 == index2:
                        self.log_entries.append("Waypoint labels " + list_label_1 + " and " \
                                                + list_label_2 + " not found in line: " + line)
                    else:
                        log_entry = "Waypoint label "
                        log_entry += list_label_1 if index1 is None else list_label_2
                        log_entry += " not found in line: " + line
                        self.log_entries.append(log_entry)
                    if invchar:
                        self.log_entries[-1] += " [contains invalid character(s)]"
                    if r not in self.routes:
                        self.routes.add(r)
                        if r.last_update and self.update and r.last_update[0] >= self.update:
                            self.log_entries.append("Route updated " + r.last_update[0] + ": " + r.readable_name())
                    continue
                # are either of the labels used duplicates?
                duplicate = False
                if list_label_1 in r.duplicate_labels:
                    log_entry = r.region + ": duplicate label " + list_label_1 + " in " + r.root \
                              + ". Please report this error in the TravelMapping forum" \
                              + ". Unable to parse line: " + line
                    self.log_entries.append(log_entry)
                    duplicate = True
                if list_label_2 in r.duplicate_labels:
                    log_entry = r.region + ": duplicate label " + list_label_2 + " in " + r.root \
                              + ". Please report this error in the TravelMapping forum" \
                              + ". Unable to parse line: " + line
                    self.log_entries.append(log_entry)
                    duplicate = True
                if duplicate:
                    if r not in self.routes:
                        self.routes.add(r)
                        if r.last_update and self.update and r.last_update[0] >= self.update:
                            self.log_entries.append("Route updated " + r.last_update[0] + ": " + r.readable_name())
                    continue
                # if both labels reference the same waypoint...
                if index1 == index2:
                    self.log_entries.append("Equivalent waypoint labels mark zero distance traveled in line: " + line)
                    if r not in self.routes:
                        self.routes.add(r)
                        if r.last_update and self.update and r.last_update[0] >= self.update:
                            self.log_entries.append("Route updated " + r.last_update[0] + ": " + r.readable_name())
                # otherwise both labels are valid; mark in use & proceed
                else:
                    r.system.listnamesinuse.add(lookup)
                    r.system.unusedaltroutenames.discard(lookup)
                    r.labels_in_use.add(list_label_1)
                    r.labels_in_use.add(list_label_2)
                    r.unused_alt_labels.discard(list_label_1)
                    r.unused_alt_labels.discard(list_label_2)
                    list_entries += 1
                    if index1 > index2:
                        (index1, index2) = (index2, index1)
                    r.store_traveled_segments(self, index1, index2)
            elif len(fields) == 6:
                lookup1 = fields[0].upper() + ' ' + fields[1].upper()
                lookup2 = fields[3].upper() + ' ' + fields[4].upper()
                # look for region/route combos, first in pri_list_hash
                # and then if not found, in alt_list_hash
                both_lookups_found = True
                try:
                    r1 = Route.pri_list_hash[lookup1]
                except KeyError:
                    try:
                        r1 = Route.alt_list_hash[lookup1]
                    except KeyError:
                        r1 = None
                    else:
                        self.log_entries.append("Note: deprecated route name \"" + fields[0] + ' ' + fields[1] \
                                              + "\" -> canonical name \"" + r1.readable_name() + "\" in line: " + line)
                try:
                    r2 = Route.pri_list_hash[lookup2]
                except KeyError:
                    try:
                        r2 = Route.alt_list_hash[lookup2]
                    except KeyError:
                        r2 = None
                    else:
                        self.log_entries.append("Note: deprecated route name \"" + fields[3] + ' ' + fields[4] \
                                              + "\" -> canonical name \"" + r2.readable_name() + "\" in line: " + line)
                if r1 is None or r2 is None:
                    (lookup1, invchar) = no_control_chars(lookup1)
                    (lookup2, invchar) = no_control_chars(lookup2)
                    (line, invchar) = no_control_chars(line)
                    if r1 == r2:
                        self.log_entries.append("Unknown region/highway combos " + lookup1 + " and " + lookup2 + " in line: " + line)
                    else:
                        log_entry = "Unknown region/highway combo "
                        log_entry += lookup1 if r1 is None else lookup2
                        log_entry += " in line: " + line
                        self.log_entries.append(log_entry)
                    if invchar:
                        self.log_entries[-1] += " [contains invalid character(s)]"
                    continue
                if r1.con_route != r2.con_route:
                    self.log_entries.append(lookup1 + " and " + lookup2 + " not in same connected route in line: " + line)
                    # log updates for routes beginning/ending r1's ConnectedRoute
                    cr = r1.con_route.roots[0]
                    if cr not in self.routes:
                        self.routes.add(cr)
                        if cr.last_update and self.update and cr.last_update[0] >= self.update:
                            self.log_entries.append("Route updated " + cr.last_update[0] + ": " + cr.readable_name())
                        if len(r1.con_route.roots) > 1:
                            cr = r1.con_route.roots[-1]
                            if cr not in self.routes:
                                self.routes.add(cr)
                                if cr.last_update and self.update and cr.last_update[0] >= self.update:
                                    self.log_entries.append("Route updated " + cr.last_update[0] + ": " + cr.readable_name())
                    # log updates for routes beginning/ending r2's ConnectedRoute
                    cr = r2.con_route.roots[0]
                    if cr not in self.routes:
                        self.routes.add(cr)
                        if cr.last_update and self.update and cr.last_update[0] >= self.update:
                            self.log_entries.append("Route updated " + cr.last_update[0] + ": " + cr.readable_name())
                        if len(r2.con_route.roots) > 1:
                            cr = r2.con_route.roots[-1]
                            if cr not in self.routes:
                                self.routes.add(cr)
                                if cr.last_update and self.update and cr.last_update[0] >= self.update:
                                    self.log_entries.append("Route updated " + cr.last_update[0] + ": " + cr.readable_name())
                    continue
                if r1.system.devel():
                    self.log_entries.append("Ignoring line matching highway in system in development: " + line)
                    continue
                # r1 and r2 are route matches, and we need to find
                # waypoint indices, ignoring case and leading
                # '+' or '*' when matching
                list_label_1 = fields[2].lstrip("+*").upper()
                list_label_2 = fields[5].lstrip("+*").upper()
                # look for point indices for labels, first in pri_label_hash
                # and then if not found, in alt_label_hash
                try:
                    index1 = r1.pri_label_hash[list_label_1]
                except KeyError:
                    try:
                        index1 = r1.alt_label_hash[list_label_1]
                    except KeyError:
                        index1 = None
                try:
                    index2 = r2.pri_label_hash[list_label_2]
                except KeyError:
                    try:
                        index2 = r2.alt_label_hash[list_label_2]
                    except KeyError:
                        index2 = None
                # if we did not find matches for both labels...
                if index1 is None or index2 is None:
                    (list_label_1, invchar) = no_control_chars(list_label_1)
                    (list_label_2, invchar) = no_control_chars(list_label_2)
                    (line, invchar) = no_control_chars(line)
                    if index1 is None and index2 is None:
                        self.log_entries.append("Waypoint labels " + list_label_1 + " and " + list_label_2 + " not found in line: " + line)
                    else:
                        log_entry = "Waypoint "
                        if index1 is None:
                            log_entry += lookup1 + ' ' + list_label_1
                        else:
                            log_entry += lookup2 + ' ' + list_label_2
                        log_entry += " not found in line: " + line
                        self.log_entries.append(log_entry)
                    if invchar:
                        self.log_entries[-1] += " [contains invalid character(s)]"
                    continue
                # are either of the labels used duplicates?
                duplicate = False
                if list_label_1 in r1.duplicate_labels:
                    self.log_entries.append(r1.region + ": duplicate label " + list_label_1 + " in " + r1.root \
                                          + ". Please report this error in the TravelMapping forum. Unable to parse line: " + line)
                    duplicate = True
                if list_label_2 in r2.duplicate_labels:
                    self.log_entries.append(r2.region + ": duplicate label " + list_label_2 + " in " + r2.root \
                                          + ". Please report this error in the TravelMapping forum. Unable to parse line: " + line)
                    duplicate = True
                if duplicate:
                    continue
                # if both region/route combos point to the same chopped route...
                if r1 == r2:
                    # if both labels reference the same waypoint...
                    if index1 == index2:
                        self.log_entries.append("Equivalent waypoint labels mark zero distance traveled in line: " + line)
                        if r1 not in self.routes:
                            self.routes.add(r1)
                            if r1.last_update and self.update and r1.last_update[0] >= self.update:
                                self.log_entries.append("Route updated " + r1.last_update[0] + ": " + r1.readable_name())
                        continue
                    if index1 <= index2:
                        r1.store_traveled_segments(self, index1, index2)
                    else:
                        r1.store_traveled_segments(self, index2, index1)
                else:
                    if r1.rootOrder > r2.rootOrder:
                        (r1, r2) = (r2, r1)
                        (index1, index2) = (index2, index1)
                    # mark the beginning chopped route from index1 to its end
                    if r1.is_reversed:
                        r1.store_traveled_segments(self, 0, index1)
                    else:
                        r1.store_traveled_segments(self, index1, len(r1.segment_list))
                    # mark the ending chopped route from its beginning to index2
                    if r2.is_reversed:
                        r2.store_traveled_segments(self, index2, len(r2.segment_list))
                    else:
                        r2.store_traveled_segments(self, 0, index2)
                    # mark any intermediate chopped routes in their entirety.
                    for r in range(r1.rootOrder+1, r2.rootOrder):
                        r1.con_route.roots[r].store_traveled_segments(self, 0, len(r1.con_route.roots[r].segment_list))
                # both labels are valid; mark in use & proceed
                r1.system.listnamesinuse.add(lookup1)
                r1.system.unusedaltroutenames.discard(lookup1)
                r1.labels_in_use.add(list_label_1)
                r1.unused_alt_labels.discard(list_label_1)
                r2.system.listnamesinuse.add(lookup2)
                r2.system.unusedaltroutenames.discard(lookup2)
                r2.labels_in_use.add(list_label_2)
                r2.unused_alt_labels.discard(list_label_2)
                list_entries += 1
            else:
                self.log_entries.append("Incorrect format line (4 or 6 fields expected, found " + \
                                        str(len(fields)) +"): " + line)

        self.log_entries.append("Processed " + str(list_entries) + \
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

class DatacheckEntry:
    """This class encapsulates a datacheck log entry

    route is a reference to the route with a datacheck error

    labels is a list of labels that are related to the error (such
    as the endpoints of a too-long segment or the three points that
    form a sharp angle)

    code is the error code | info is additional
    string, one of:        | information, if used:
    -----------------------+--------------------------------------------
    BAD_ANGLE              |
    BUS_WITH_I             |
    DISCONNECTED_ROUTE     | adjacent root's expected connection point
    DUPLICATE_COORDS       | coordinate pair
    DUPLICATE_LABEL        |
    HIDDEN_JUNCTION        | number of incident edges in TM master graph
    HIDDEN_TERMINUS        |
    INTERSTATE_NO_HYPHEN   |
    INVALID_FINAL_CHAR     | final character in label
    INVALID_FIRST_CHAR     | first character in label other than *
    LABEL_INVALID_CHAR     |
    LABEL_LOOKS_HIDDEN     |
    LABEL_PARENS           |
    LABEL_SELFREF          |
    LABEL_SLASHES          |
    LABEL_TOO_LONG         |
    LABEL_UNDERSCORES      |
    LACKS_GENERIC          |
    LONG_SEGMENT           | distance in miles
    LONG_UNDERSCORE        |
    MALFORMED_LAT          | malformed "lat=" parameter from OSM url
    MALFORMED_LON          | malformed "lon=" parameter from OSM url
    MALFORMED_URL          | always "MISSING_ARG(S)"
    NONTERMINAL_UNDERSCORE |
    OUT_OF_BOUNDS          | coordinate pair
    SHARP_ANGLE            | angle in degrees
    US_LETTER              |
    VISIBLE_DISTANCE       | distance in miles
    VISIBLE_HIDDEN_COLOC   | hidden point at same coordinates

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

class HGVertex:
    """This class encapsulates information needed for a highway graph
    vertex.
    """

    def __init__(self,wpt,unique_name,datacheckerrors,rg_vset_hash):
        self.lat = wpt.lat
        self.lng = wpt.lng
        self.unique_name = unique_name
        self.visibility = 0
            # permitted values:
            # 0: never visible outside of simple graphs
            # 1: visible only in traveled graph; hidden in collapsed graph
            # 2: visible in both traveled & collapsed graphs
        self.incident_s_edges = [] # simple
        self.incident_c_edges = [] # collapsed
        self.incident_t_edges = [] # traveled
        if wpt.colocated is None:
            if not wpt.is_hidden:
                self.visibility = 2
            wpt.route.system.vertices.add(self)
            if wpt.route.region not in rg_vset_hash:
                rg_vset_hash[wpt.route.region] = set()
                rg_vset_hash[wpt.route.region].add(self)
            else:
                rg_vset_hash[wpt.route.region].add(self)
            return
        for w in wpt.colocated:
            # will consider hidden iff all colocated waypoints are hidden
            if not w.is_hidden:
                self.visibility = 2
            if w.route.region not in rg_vset_hash:
                rg_vset_hash[w.route.region] = set()
                rg_vset_hash[w.route.region].add(self)
            else:
                rg_vset_hash[w.route.region].add(self)
            w.route.system.vertices.add(self)
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

class HGEdge:
    """This class encapsulates information needed for a highway graph edge.
    """

    def __init__(self,s=None,graph=None,vertex=None,fmt_mask=None):
        if s is None and vertex is None:
            print("ERROR: improper use of HGEdge constructor: s is None; vertex is None\n")
            return

        # a few items we can do for either construction type
        self.s_written = False # simple
        self.c_written = False # collapsed
        self.t_written = False # traveled

        # intermediate points, if more than 1, will go from vertex1 to
        # vertex2
        self.intermediate_points = []

        # initial construction is based on a HighwaySegment
        if s is not None:
            if graph is None:
                print("ERROR: improper use of HGEdge constructor: s is not None; graph is None\n")
                return
            self.segment_name = s.segment_name()
            self.vertex1 = graph.vertices[s.waypoint1.hashpoint()]
            self.vertex2 = graph.vertices[s.waypoint2.hashpoint()]
            # canonical segment, used to reference region and list of travelers
            # assumption: each edge/segment lives within a unique region
            # and a 'multi-edge' would not be able to span regions as there
            # would be a required visible waypoint at the border
            self.segment = s
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
            for e in self.vertex1.incident_s_edges:
                if e.vertex1 == self.vertex2 and e.vertex2 == self.vertex1:
                    return
            for e in self.vertex2.incident_s_edges:
                if e.vertex1 == self.vertex2 and e.vertex2 == self.vertex1:
                    return
            self.vertex1.incident_s_edges.append(self)
            self.vertex2.incident_s_edges.append(self)
            self.vertex1.incident_c_edges.append(self)
            self.vertex2.incident_c_edges.append(self)
            self.vertex1.incident_t_edges.append(self)
            self.vertex2.incident_t_edges.append(self)

        # build by collapsing two existing edges around a common
        # hidden vertex waypoint, whose information is given in
        # vertex
        if vertex is not None:
            # we know there are exactly 2 incident edges, as we
            # checked for that, and we will replace these two
            # with the single edge we are constructing here...
            if fmt_mask & 1 == 1:
                # ...in the compressed graph, and/or...
                edge1 = vertex.incident_c_edges[0]
                edge2 = vertex.incident_c_edges[1]
            if fmt_mask & 2 == 2:
                # ...in the traveled graph, as appropriate
                edge1 = vertex.incident_t_edges[0]
                edge2 = vertex.incident_t_edges[1]
            # segment names should match as routes should not start or end
            # nor should concurrencies begin or end at a hidden point
            if edge1.segment_name != edge2.segment_name:
                print("ERROR: segment name mismatch in HGEdge collapse constructor: edge1 named " + edge1.segment_name + " edge2 named " + edge2.segment_name + "\n")
            self.segment_name = edge1.segment_name
            #print("\nDEBUG: collapsing edges along " + self.segment_name + " at vertex " + str(vertex) + ", edge1 is " + str(edge1) + " and edge2 is " + str(edge2))
            # segment and route names/systems should also match, but not
            # doing that sanity check here, as the above check should take
            # care of that
            self.segment = edge1.segment
            self.route_names_and_systems = edge1.route_names_and_systems

            # figure out and remember which endpoints are not the
            # vertex we are collapsing and set them as our new
            # endpoints, and at the same time, build up our list of
            # intermediate vertices
            self.intermediate_points = edge1.intermediate_points.copy()
            #print("DEBUG: copied edge1 intermediates" + self.intermediate_point_string())

            if edge1.vertex1 == vertex:
                #print("DEBUG: self.vertex1 getting edge1.vertex2: " + str(edge1.vertex2) + " and reversing edge1 intermediates")
                self.vertex1 = edge1.vertex2
                self.intermediate_points.reverse()
            else:
                #print("DEBUG: self.vertex1 getting edge1.vertex1: " + str(edge1.vertex1))
                self.vertex1 = edge1.vertex1

            #print("DEBUG: appending to intermediates: " + str(vertex))
            self.intermediate_points.append(vertex)

            toappend = edge2.intermediate_points.copy()
            #print("DEBUG: copied edge2 intermediates" + edge2.intermediate_point_string())
            if edge2.vertex1 == vertex:
                #print("DEBUG: self.vertex2 getting edge2.vertex2: " + str(edge2.vertex2))
                self.vertex2 = edge2.vertex2
            else:
                #print("DEBUG: self.vertex2 getting edge2.vertex1: " + str(edge2.vertex1) + " and reversing edge2 intermediates")
                self.vertex2 = edge2.vertex1
                toappend.reverse()

            self.intermediate_points.extend(toappend)

            #print("DEBUG: intermediates complete: from " + str(self.vertex1) + " via " + self.intermediate_point_string() + " to " + str(self.vertex2))

            # replace edge references at our endpoints with ourself
            if fmt_mask & 1 == 1: # collapsed edges
                removed = 0
                if edge1 in self.vertex1.incident_c_edges:
                    self.vertex1.incident_c_edges.remove(edge1)
                    removed += 1
                if edge1 in self.vertex2.incident_c_edges:
                    self.vertex2.incident_c_edges.remove(edge1)
                    removed += 1
                if removed != 1:
                    print("ERROR: collapsed edge1 " + str(edge1) + " removed from " + str(removed) + " adjacency lists instead of 1.")
                removed = 0
                if edge2 in self.vertex1.incident_c_edges:
                    self.vertex1.incident_c_edges.remove(edge2)
                    removed += 1
                if edge2 in self.vertex2.incident_c_edges:
                    self.vertex2.incident_c_edges.remove(edge2)
                    removed += 1
                if removed != 1:
                    print("ERROR: collapsed edge2 " + str(edge2) + " removed from " + str(removed) + " adjacency lists instead of 1.")
                self.vertex1.incident_c_edges.append(self)
                self.vertex2.incident_c_edges.append(self)
            if fmt_mask & 2 == 2: # traveled edges
                removed = 0
                if edge1 in self.vertex1.incident_t_edges:
                    self.vertex1.incident_t_edges.remove(edge1)
                    removed += 1
                if edge1 in self.vertex2.incident_t_edges:
                    self.vertex2.incident_t_edges.remove(edge1)
                    removed += 1
                if removed != 1:
                    print("ERROR: traveled edge1 " + str(edge1) + " removed from " + str(removed) + " adjacency lists instead of 1.")
                removed = 0
                if edge2 in self.vertex1.incident_t_edges:
                    self.vertex1.incident_t_edges.remove(edge2)
                    removed += 1
                if edge2 in self.vertex2.incident_t_edges:
                    self.vertex2.incident_t_edges.remove(edge2)
                    removed += 1
                if removed != 1:
                    print("ERROR: traveled edge2 " + str(edge2) + " removed from " + str(removed) + " adjacency lists instead of 1.")
                self.vertex1.incident_t_edges.append(self)
                self.vertex2.incident_t_edges.append(self)

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
        return "HGEdge: " + self.segment_name + " from " + str(self.vertex1) + " to " + str(self.vertex2) + " via " + str(len(self.intermediate_points)) + " points"

    # line appropriate for a tmg collapsed edge file
    def collapsed_tmg_line(self, systems=None):
        line = str(self.vertex1.c_vertex_num) + " " + str(self.vertex2.c_vertex_num) + " " + self.label(systems)
        for intermediate in self.intermediate_points:
            line += " " + str(intermediate.lat) + " " + str(intermediate.lng)
        return line

    # line appropriate for a tmg traveled edge file
    def traveled_tmg_line(self, traveler_lists, systems=None):
        line = str(self.vertex1.t_vertex_num) + " " + str(self.vertex2.t_vertex_num) + " " + self.label(systems)
        line += " " + self.segment.clinchedby_code(traveler_lists)
        for intermediate in self.intermediate_points:
            line += " " + str(intermediate.lat) + " " + str(intermediate.lng)
        return line

    # line appropriate for a tmg collapsed edge file, with debug info
    def debug_tmg_line(self, systems=None):
        line = str(self.vertex1.c_vertex_num) + " [" + self.vertex1.unique_name + "] " + str(self.vertex2.c_vertex_num) + " [" + self.vertex2.unique_name + "] " + self.label(systems)
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

    def __init__(self, descr, title, lat, lng, r):
        self.descr = descr
        self.title = title
        self.lat = lat
        self.lng = lng
        self.r = r

    def contains_vertex(self, v):
        """return whether v's coordinates are within this area"""
        # convert to radians to compute distance
        rlat1 = math.radians(self.lat)
        rlng1 = math.radians(self.lng)
        rlat2 = math.radians(v.lat)
        rlng2 = math.radians(v.lng)

        # original formula
        """ans = math.acos(math.cos(rlat1)*math.cos(rlng1)*math.cos(rlat2)*math.cos(rlng2) +\
                        math.cos(rlat1)*math.sin(rlng1)*math.cos(rlat2)*math.sin(rlng2) +\
                        math.sin(rlat1)*math.sin(rlat2)) * 3963.1 # EARTH_RADIUS """

        # spherical law of cosines formula (same as orig, with some terms factored out or removed via trig identity)
        ans = math.acos(math.cos(rlat1)*math.cos(rlat2)*math.cos(rlng2-rlng1)+math.sin(rlat1)*math.sin(rlat2)) * 3963.1 # EARTH_RADIUS """

        # Vincenty formula
        """ans = math.atan\
        (   math.sqrt(pow(math.cos(rlat2)*math.sin(rlng2-rlng1),2)+pow(math.cos(rlat1)*math.sin(rlat2)-math.sin(rlat1)*math.cos(rlat2)*math.cos(rlng2-rlng1),2))
            /
            (math.sin(rlat1)*math.sin(rlat2)+math.cos(rlat1)*math.cos(rlat2)*math.cos(rlng2-rlng1))
        ) * 3963.1 # EARTH_RADIUS """

        # haversine formula
        """ans = math.asin(math.sqrt(pow(math.sin((rlat2-rlat1)/2),2) + math.cos(rlat1) * math.cos(rlat2) * pow(math.sin((rlng2-rlng1)/2),2))) * 7926.2 # EARTH_DIAMETER """

        return ans <= self.r

    def contains_edge(self, e):
        """return whether both endpoints of edge e are within this area"""
        return (self.contains_vertex(e.vertex1) and
                self.contains_vertex(e.vertex2))
        
    def vertices(self, qt, g):
        # Compute and return a set of graph vertices within r miles of (lat, lng).
        # This function handles setup & sanity checks, passing control over
        # to the recursive v_search function to do the actual searching.

        # N/S sanity check: If lat is <= r/2 miles to the N or S pole, lngdelta calculation will fail.
        # In these cases, our place radius will span the entire "width" of the world, from -180 to +180 degrees.
        if math.radians(90-abs(self.lat)) <= self.r/7926.2:
            return self.v_search(qt, g, -180, +180)

        # width, in degrees longitude, of our bounding box for quadtree search
        rlat = math.radians(self.lat)
        lngdelta = math.degrees(math.acos((math.cos(self.r/3963.1) - math.sin(rlat)**2) / math.cos(rlat)**2))
        w_bound = self.lng-lngdelta
        e_bound = self.lng+lngdelta

        # normal operation; search quadtree within calculated bounds
        vertex_set = self.v_search(qt, g, w_bound, e_bound);

        # If bounding box spans international date line to west of -180 degrees,
        # search quadtree within the corresponding range of positive longitudes
        if w_bound <= -180:
            while w_bound <= -180:
                w_bound += 360
            vertex_set = vertex_set | self.v_search(qt, g, w_bound, 180)

        # If bounding box spans international date line to east of +180 degrees,
        # search quadtree within the corresponding range of negative longitudes
        if e_bound >= 180:
            while e_bound >= 180:
                e_bound -= 360
            vertex_set = vertex_set | self.v_search(qt, g, -180, e_bound)

        return vertex_set

    def v_search(self, qt, g, w_bound, e_bound):
        # recursively search quadtree for waypoints within this PlaceRadius area, and return a set
        # of their corresponding graph vertices to return to the PlaceRadius.vertices function
        vertex_set = set()

        # first check if this is a terminal quadrant, and if it is,
        # we search for vertices within this quadrant
        if qt.points is not None:
            for p in qt.points:
                if (p.colocated is None or p == p.colocated[0]) and p.is_or_colocated_with_active_or_preview() and self.contains_vertex(p):
                    vertex_set.add(g.vertices[p])
        # if we're not a terminal quadrant, we need to determine which
        # of our child quadrants we need to search and recurse into each
        else:
            #print("DEBUG: recursive case, mid_lat="+str(qt.mid_lat)+" mid_lng="+str(qt.mid_lng), flush=True)
            look_n = (self.lat + math.degrees(self.r/3963.1)) >= qt.mid_lat
            look_s = (self.lat - math.degrees(self.r/3963.1)) <= qt.mid_lat
            look_e = e_bound >= qt.mid_lng
            look_w = w_bound <= qt.mid_lng
            #print("DEBUG: recursive case, " + str(look_n) + " " + str(look_s) + " " + str(look_e) + " " + str(look_w))
            # now look in the appropriate child quadrants
            if look_n and look_w:
                vertex_set = vertex_set | self.v_search(qt.nw_child, g, w_bound, e_bound)
            if look_n and look_e:
                vertex_set = vertex_set | self.v_search(qt.ne_child, g, w_bound, e_bound)
            if look_s and look_w:
                vertex_set = vertex_set | self.v_search(qt.sw_child, g, w_bound, e_bound)
            if look_s and look_e:
                vertex_set = vertex_set | self.v_search(qt.se_child, g, w_bound, e_bound)
        return vertex_set;

class HighwayGraph:
    """This class implements the capability to create graph
    data structures representing the highway data.

    On construction, build a set of unique vertex names
    and determine edges, at most one per concurrent segment.
    Create three sets of edges:
     - one for the simple graph
     - one for the collapsed graph with hidden waypoints compressed into multi-point edges
     - one for the traveled graph: collapsed edges split at endpoints of users' travels
    """

    def __init__(self, all_waypoints, highway_systems, datacheckerrors, et):
        # first, find unique waypoints and create vertex labels
        vertex_names = set()
        self.vertices = {}
        # hash table containing a set of vertices for each region
        self.rg_vset_hash = {}
        hi_priority_points = []
        lo_priority_points = []
        all_waypoints.graph_points(hi_priority_points, lo_priority_points)

        # to track the waypoint name compressions, add log entries
        # to this list
        self.waypoint_naming_log = []

        # loop for each Waypoint, create a unique name and vertex
        # unless it's a point not in or colocated with any active
        # or preview system, or is colocated and not at the front
        # of its colocation list
        counter = 0
        print(et.et() + "Creating unique names and vertices", end="", flush=True)
        for w in hi_priority_points + lo_priority_points:
            if counter % 10000 == 0:
                print('.', end="", flush=True)
            counter += 1

            # come up with a unique name that brings in its meaning

            # start with the canonical name
            point_name = w.canonical_waypoint_name(self.waypoint_naming_log, vertex_names, datacheckerrors)

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
            self.vertices[w] = HGVertex(w, point_name, datacheckerrors, self.rg_vset_hash)

            # active/preview colocation lists are no longer needed; clear them
            del w.ap_coloc

        # now that vertices are in place with names, set of unique names is no longer needed
        del vertex_names

        # create edges
        counter = 0
        print("!\n" + et.et() + "Creating edges", end="", flush=True)
        for h in highway_systems:
            if h.level != "active" and h.level != "preview":
                continue
            if counter % 6 == 0:
                print('.', end="", flush=True)
            counter += 1;
            for r in h.route_list:
                for s in r.segment_list:
                    if s.concurrent is None or s == s.concurrent[0]:
                        HGEdge(s, self)

        # compress edges adjacent to hidden vertices
        counter = 0
        print("!\n" + et.et() + "Compressing collapsed edges", end="", flush=True)
        for w, v in self.vertices.items():
            if counter % 10000 == 0:
                print('.', end="", flush=True)
            counter += 1
            if v.visibility == 0:
                # cases with only one edge are flagged as HIDDEN_TERMINUS
                if len(v.incident_c_edges) < 2:
                    v.visibility = 2
                    continue
                # if >2 edges, flag HIDDEN_JUNCTION, mark as visible, and do not compress
                if len(v.incident_c_edges) > 2:
                    datacheckerrors.append(DatacheckEntry(w.colocated[0].route,[w.colocated[0].label],
                                           "HIDDEN_JUNCTION",str(len(v.incident_c_edges))))
                    v.visibility = 2
                    continue
                # if edge clinched_by sets mismatch, set visibility to 1
                # (visible in traveled graph; hidden in collapsed graph)
                # first, the easy check, for whether list sizes mismatch
                if len(v.incident_t_edges[0].segment.clinched_by) \
                != len(v.incident_t_edges[1].segment.clinched_by):
                        v.visibility = 1
                # next, compare clinched_by lists; look for any element in the 1st not in the 2nd
                else:
                    for t in v.incident_t_edges[0].segment.clinched_by:
                        if t not in v.incident_t_edges[1].segment.clinched_by:
                            v.visibility = 1
                            break
                # construct from vertex this time
                if v.visibility == 1:
                    HGEdge(vertex=v, fmt_mask=1)
                else:
                    if (v.incident_c_edges[0] == v.incident_t_edges[0] and v.incident_c_edges[1] == v.incident_t_edges[1]) \
                    or (v.incident_c_edges[0] == v.incident_t_edges[1] and v.incident_c_edges[1] == v.incident_t_edges[0]):
                        HGEdge(vertex=v, fmt_mask=3)
                    else:
                        HGEdge(vertex=v, fmt_mask=1)
                        HGEdge(vertex=v, fmt_mask=2)
        print("!")

    def matching_vertices_and_edges(self, qt, regions, systems, placeradius, rg_vset_hash):
        # return seven items:
        # mvset		 # a set of vertices, optionally restricted by region or system or placeradius area
        cv_count = 0	 # the number of collapsed vertices in this set
        tv_count = 0	 # the number of traveled vertices in this set
        mse = set()	 # matching    simple edges
        mce = set()	 # matching collapsed edges
        mte = set()	 # matching  traveled edges
        trav_set = set() # sorted into a list of travelers for traveled graphs
        # ...as a tuple

        rvset = set()	# union of all sets in regions
        svset = set()	# union of all sets in systems
        if regions is not None:
            for r in regions:
                rvset = rvset | rg_vset_hash[r]
        if systems is not None:
            for h in systems:
                svset = svset | h.vertices
        if placeradius is not None:
            pvset = placeradius.vertices(qt, self)

        # determine which vertices are within our region(s) and/or system(s)
        if regions is not None:
            mvset = rvset
            if placeradius is not None:
                mvset = mvset & pvset
            if systems is not None:
                mvset = mvset & svset
        elif systems is not None:
            mvset = svset
            if placeradius is not None:
                mvset = mvset & pvset
        elif placeradius is not None:
            mvset = pvset
        else: # no restrictions via region, system, or placeradius, so include everything
            mvset = set()
            for v in self.vertices.values():
                mvset.add(v)
        
        # Compute sets of edges for subgraphs, optionally
        # restricted by region or system or placeradius.
        # Keep a count of collapsed & traveled vertices as we go.
        for v in mvset:
            for e in v.incident_s_edges:
                if placeradius is None or placeradius.contains_edge(e):
                    if regions is None or e.segment.route.region in regions:
                        system_match = systems is None
                        if not system_match:
                            for (r, s) in e.route_names_and_systems:
                                if s in systems:
                                    system_match = True
                                    break
                        if system_match:
                            mse.add(e)
            if v.visibility < 1:
                continue
            tv_count += 1
            for e in v.incident_t_edges:
                if placeradius is None or placeradius.contains_edge(e):
                    if regions is None or e.segment.route.region in regions:
                        system_match = systems is None
                        if not system_match:
                            for (r, s) in e.route_names_and_systems:
                                if s in systems:
                                    system_match = True
                                    break
                        if system_match:
                            mte.add(e)
                            for t in e.segment.clinched_by:
                                trav_set.add(t)
            if v.visibility < 2:
                continue
            cv_count += 1
            for e in v.incident_c_edges:
                if placeradius is None or placeradius.contains_edge(e):
                    if regions is None or e.segment.route.region in regions:
                        system_match = systems is None
                        if not system_match:
                            for (r, s) in e.route_names_and_systems:
                                if s in systems:
                                    system_match = True
                                    break
                        if system_match:
                            mce.add(e)
        return (mvset, cv_count, tv_count, mse, mce, mte, sorted(trav_set, key=lambda TravelerList: TravelerList.traveler_name))

    # write the entire set of highway data in .tmg format.
    # The first line is a header specifying the format and version number,
    # The second line specifies the number of waypoints, w, the number of connections, c,
    #     and for traveled graphs only, the number of travelers.
    # Then, w lines describing waypoints (label, latitude, longitude).
    # Then, c lines describing connections (endpoint 1 number, endpoint 2 number, route label),
    #     followed on traveled graphs only by a hexadecimal code encoding travelers on that segment,
    #     followed on both collapsed & traveled graphs by a list of latitude & longitude values
    #     for intermediate "shaping points" along the edge, ordered from endpoint 1 to endpoint 2.
    #
    def write_master_graphs_tmg(self, graph_list, path, traveler_lists):
        simplefile = open(path+"tm-master-simple.tmg","w",encoding='utf-8')
        collapfile = open(path+"tm-master-collapsed.tmg","w",encoding='utf-8')
        travelfile = open(path+"tm-master-traveled.tmg","w",encoding='utf-8')
        cv = 0
        tv = 0
        se = 0
        ce = 0
        te = 0

        # count vertices & edges
        for v in self.vertices.values():
            se += len(v.incident_s_edges)
            if v.visibility >= 1:
                tv += 1
                te += len(v.incident_t_edges)
                if v.visibility == 2:
                    cv += 1
                    ce += len(v.incident_c_edges)
        se //= 2;
        ce //= 2;
        te //= 2;

        # write graph headers
        simplefile.write("TMG 1.0 simple\n")
        collapfile.write("TMG 1.0 collapsed\n")
        travelfile.write("TMG 2.0 traveled\n")
        simplefile.write(str(len(self.vertices)) + ' ' + str(se) + '\n')
        collapfile.write(str(cv) + ' ' + str(ce) + '\n')
        travelfile.write(str(tv) + ' ' + str(te) + ' ' + str(len(traveler_lists)) + '\n')

        # write vertices
        sv = 0
        cv = 0
        tv = 0
        for v in self.vertices.values():
            vstr = v.unique_name+' '+str(v.lat)+' '+str(v.lng)+'\n'
            # all vertices for simple graph
            simplefile.write(vstr)
            v.s_vertex_num = sv
            sv += 1
            # visible vertices...
            if v.visibility >= 1:
                # for traveled graph,
                travelfile.write(vstr)
                v.t_vertex_num = tv
                tv += 1
                if v.visibility == 2:
                    # and for collapsed graph
                    collapfile.write(vstr)
                    v.c_vertex_num = cv
                    cv += 1
        # now edges, only write if not already written
        for v in self.vertices.values():
            for e in v.incident_s_edges:
                if not e.s_written:
                    e.s_written = 1
                    simplefile.write(str(e.vertex1.s_vertex_num) + ' ' + str(e.vertex2.s_vertex_num) + ' ' + e.label() + '\n')
            # write edges if vertex is visible...
            if v.visibility >= 1:
                # in traveled graph,
                for e in v.incident_t_edges:
                    if not e.t_written:
                        e.t_written = 1
                        travelfile.write(e.traveled_tmg_line(traveler_lists) + '\n')
                if v.visibility == 2:
                    # and in collapsed graph
                    for e in v.incident_c_edges:
                        if not e.c_written:
                            e.c_written = 1
                            collapfile.write(e.collapsed_tmg_line() + '\n')
        # traveler names
        for t in traveler_lists:
            travelfile.write(t.traveler_name + ' ')
        travelfile.write('\n')
        simplefile.close()
        collapfile.close()
        travelfile.close()
        graph_list.append(GraphListEntry('tm-master-simple.tmg', 'All Travel Mapping Data', sv, se, 0, 'simple', 'master'))
        graph_list.append(GraphListEntry('tm-master-collapsed.tmg', 'All Travel Mapping Data', cv, ce, 0, 'collapsed', 'master'))
        graph_list.append(GraphListEntry('tm-master-traveled.tmg', 'All Travel Mapping Data', tv, te, len(traveler_lists), 'traveled', 'master'))
        # print summary info
        print("   Simple graph has " + str(len(self.vertices)) +
              " vertices, " + str(se) + " edges.")
        print("Collapsed graph has " + str(cv) +
              " vertices, " + str(ce) + " edges.")
        print(" Traveled graph has " + str(tv) +
              " vertices, " + str(te) + " edges.")

    # write a subset of the data,
    # in simple, collapsed and traveled formats,
    # restricted by regions in the list if given,
    # by systems in the list if given,
    # or to within a given area if placeradius is given
    def write_subgraphs_tmg(self, graph_list, path, root, descr, category, regions, systems, placeradius, qt):
        simplefile = open(path+root+"-simple.tmg","w",encoding='utf-8')
        collapfile = open(path+root+"-collapsed.tmg","w",encoding='utf-8')
        travelfile = open(path+root+"-traveled.tmg","w",encoding='utf-8')
        (mv, cv_count, tv_count, mse, mce, mte, traveler_lists) = self.matching_vertices_and_edges(qt, regions, systems, placeradius, self.rg_vset_hash)
        """if len(traveler_lists) == 0:
            print("\n\nNo travelers in " + root + "\n", flush=True)#"""
        # assign traveler numbers
        travnum = 0
        for t in traveler_lists:
            t.traveler_num = travnum
            travnum += 1
        print('(' + str(len(mv)) + ',' + str(len(mse)) + ") ", end="", flush=True)
        print('(' + str(cv_count) + ',' + str(len(mce)) + ") ", end="", flush=True)
        print('(' + str(tv_count) + ',' + str(len(mte)) + ") ", end="", flush=True)
        simplefile.write("TMG 1.0 simple\n")
        collapfile.write("TMG 1.0 collapsed\n")
        travelfile.write("TMG 2.0 traveled\n")
        simplefile.write(str(len(mv)) + ' ' + str(len(mse)) + '\n')
        collapfile.write(str(cv_count) + ' ' + str(len(mce)) + '\n')
        travelfile.write(str(tv_count) + ' ' + str(len(mte)) + ' ' + str(len(traveler_lists)) + '\n')

        # write vertices
        sv = 0
        cv = 0
        tv = 0
        for v in mv:
            vstr = v.unique_name + ' ' + str(v.lat) + ' ' + str(v.lng) + '\n'
            # all vertices, for simple graph
            simplefile.write(vstr)
            v.s_vertex_num = sv
            sv += 1
            # visible vertices
            if v.visibility >= 1:
                # for traveled graph
                travelfile.write(vstr)
                v.t_vertex_num = tv
                tv += 1
                if v.visibility == 2:
                    # for collapsed graph
                    collapfile.write(vstr)
                    v.c_vertex_num = cv
                    cv += 1
        # write edges
        for e in mse:
            simplefile.write(str(e.vertex1.s_vertex_num) + ' ' + str(e.vertex2.s_vertex_num) + ' ' + e.label(systems) + '\n')
        for e in mce:
            collapfile.write(e.collapsed_tmg_line(systems) + '\n')
        for e in mte:
            travelfile.write(e.traveled_tmg_line(traveler_lists, systems) + '\n')

        # traveler names
        for t in traveler_lists:
            travelfile.write(t.traveler_name + ' ')
        travelfile.write('\n')

        simplefile.close()
        collapfile.close()
        travelfile.close()

        graph_list.append(GraphListEntry(root+   "-simple.tmg", descr, len(mv),  len(mse), 0, "simple",    category))
        graph_list.append(GraphListEntry(root+"-collapsed.tmg", descr, cv_count, len(mce), 0, "collapsed", category))
        graph_list.append(GraphListEntry(root+ "-traveled.tmg", descr, tv_count, len(mte), len(traveler_lists), "traveled",  category))

def format_clinched_mi(clinched,total):
    """return a nicely-formatted string for a given number of miles
    clinched and total miles, including percentage"""
    percentage = "-.--%"
    if total != 0.0:
        percentage = "({0:.2f}%)".format(100*clinched/total)
    return "{0:.2f}".format(clinched) + " of {0:.2f}".format(total) + \
        " mi " + percentage

def valid_num_str(data):
    point_count = 0
    # check initial character
    if data[0] == '.':
        point_count = 1
    elif data[0] < '0' and data[0] != '-' or data[0] > '9':
        return False
    # check subsequent characters
    for c in data[1:]:
        # check for minus sign not at beginning
        if c == '-':
            return False
        # check for multiple decimal points
        if c == '.':
            point_count += 1
            if point_count > 1:
                return False
        # check for invalid characters
        elif c < '0' or c > '9':
            return False
    return True

def no_control_chars(input):
    output = ""
    invchar = False
    for c in input:
        if c < ' ' or c == '\x7F':
            output += '?'
            invchar = True
        else:
            output += c
    return (output, invchar)

class GraphListEntry:
    """This class encapsulates information about generated graphs for
    inclusion in the DB table.  Field names here match column names
    in the "graphs" DB table.
    """

    def __init__(self,filename,descr,vertices,edges,travelers,format,category):
        self.filename = filename
        self.descr = descr
        self.vertices = vertices
        self.edges = edges
        self.travelers = travelers
        self.format = format
        self.category = category
    
# 
# Execution code starts here
#
# start a timer for including elapsed time reports in messages
et = ElapsedTime()

print("Start: " + str(datetime.datetime.now()))
      
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
        line=line.strip()
        if len(line) == 0:
            continue
        fields = line.split(";")
        if len(fields) != 2:
            el.add_error("Could not parse continents.csv line: [" + line +
                         "], expected 2 fields, found " + str(len(fields)))
            continue
        # verify field lengths
        if len(fields[0].encode('utf-8')) > DBFieldLength.continentCode:
            el.add_error("Continent code > " + str(DBFieldLength.continentCode) +
                         " bytes in continents.csv line " + line)
        if len(fields[1].encode('utf-8')) > DBFieldLength.continentName:
            el.add_error("Continent name > " + str(DBFieldLength.continentName) +
                         " bytes in continents.csv line " + line)
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
        line=line.strip()
        if len(line) == 0:
            continue
        fields = line.split(";")
        if len(fields) != 2:
            el.add_error("Could not parse countries.csv line: [" + line +
                         "], expected 2 fields, found " + str(len(fields)))
            continue
        # verify field lengths
        if len(fields[0].encode('utf-8')) > DBFieldLength.countryCode:
            el.add_error("Country code > " + str(DBFieldLength.countryCode) +
                         " bytes in countries.csv line " + line)
        if len(fields[1].encode('utf-8')) > DBFieldLength.countryName:
            el.add_error("Country name > " + str(DBFieldLength.countryName) +
                         " bytes in countries.csv line " + line)
        countries.append(fields)

all_regions = {}
try:
    file = open(args.highwaydatapath+"/regions.csv", "rt",encoding='utf-8')
except OSError as e:
    el.add_error(str(e))
else:
    lines = file.readlines()
    file.close()
    lines.pop(0)  # ignore header line
    for line in lines:
        line=line.strip()
        if len(line) == 0:
            continue
        fields = line.split(";")
        if len(fields) != 5:
            el.add_error("Could not parse regions.csv line: [" + line +
                         "], expected 5 fields, found " + str(len(fields)))
            continue
        # verify field data
        if len(fields[0].encode('utf-8')) > DBFieldLength.regionCode:
            el.add_error("Region code > " + str(DBFieldLength.regionCode) +
                         " bytes in regions.csv line " + line)
        if len(fields[1].encode('utf-8')) > DBFieldLength.regionName:
            el.add_error("Region name > " + str(DBFieldLength.regionName) +
                         " bytes in regions.csv line " + line)
        c_found = False
        for c in countries:
            if c[0] == fields[2]:
                c_found = True
                break
        if not c_found:
            el.add_error("Could not find country matching regions.csv line: " + line)
        c_found = False
        for c in continents:
            if c[0] == fields[3]:
                c_found = True
                break
        if not c_found:
            el.add_error("Could not find continent matching regions.csv line: " + line)
        if len(fields[4].encode('utf-8')) > DBFieldLength.regiontype:
            el.add_error("Region type > " + str(DBFieldLength.regiontype) +
                         " bytes in regions.csv line " + line)
        all_regions[fields[0]] = fields

# Create a list of HighwaySystem objects, one per system in systems.csv file
highway_systems = []
print(et.et() + "Reading systems list in " + args.highwaydatapath+"/"+args.systemsfile + ".",flush=True)
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
        line=line.strip()
        if len(line) == 0:
            continue
        if line.startswith('#'):
            ignoring.append("Ignored comment in " + args.systemsfile + ": " + line.rstrip('\n'))
            continue
        fields = line.split(";")
        if len(fields) != 6:
            el.add_error("Could not parse " + args.systemsfile + " line: [" + line +
                         "], expected 6 fields, found " + str(len(fields)))
            continue
        # verify System
        if len(fields[0].encode('utf-8')) > DBFieldLength.systemName:
            el.add_error("System code > " + str(DBFieldLength.systemName) +
                         " bytes in " + args.systemsfile + " line " + line)
        # verify CountryCode
        valid_country = False
        for i in range(len(countries)):
            country = countries[i][0]
            if country == fields[1]:
                valid_country = True
                break
        if not valid_country:
            el.add_error("Could not find country matching " + args.systemsfile + " line: " + line)
        # verify Name
        if len(fields[2].encode('utf-8')) > DBFieldLength.systemFullName:
            el.add_error("System name > " + str(DBFieldLength.systemFullName) +
                         " bytes in " + args.systemsfile + " line " + line)
        # verify Color
        if len(fields[3].encode('utf-8')) > DBFieldLength.color:
            el.add_error("Color > " + str(DBFieldLength.color) +
                         " bytes in " + args.systemsfile + " line " + line)
        # verify Tier
        try:
            fields[4] = int(fields[4])
        except ValueError:
            el.add_error("Invalid tier in " + args.systemsfile + " line " + line)
        else:
            if fields[4] < 1:
                el.add_error("Invalid tier in " + args.systemsfile + " line " + line)
        # verify Level
        if fields[5] != "active" and fields[5] != "preview" and fields[5] != "devel":
            el.add_error("Unrecognized level in " + args.systemsfile + " line: " + line)

        print(fields[0] + ".",end="",flush=True)
        hs = HighwaySystem(fields[0], fields[1], fields[2],
                           fields[3], fields[4], fields[5], el,
                           args.highwaydatapath+"/hwy_data/_systems")
        highway_systems.append(hs)
    print("")
    # print at the end the lines ignored
    for line in ignoring:
        print(line)

# list for datacheck errors that we will need later
datacheckerrors = []

# For tracking whether any .wpt files are in the directory tree
# that do not have a .csv file entry that causes them to be
# read into the data
print(et.et() + "Finding all .wpt files. ",end="",flush=True)
all_wpt_files = set()
for dir, sub, files in os.walk(args.highwaydatapath+"/hwy_data"):
    for file in files:
        if file.endswith('.wpt') and '_boundaries' not in dir:
            all_wpt_files.add(dir+"/"+file)
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

print(et.et() + "Finding unprocessed wpt files.", flush=True)
unprocessedfile = open(args.logfilepath+'/unprocessedwpts.log','w',encoding='utf-8')
if len(all_wpt_files) > 0:
    print(str(len(all_wpt_files)) + " .wpt files in " + args.highwaydatapath +
          "/hwy_data not processed, see unprocessedwpts.log.")
    for file in sorted(all_wpt_files):
        unprocessedfile.write(file[file.find('hwy_data'):] + '\n')
else:
    print("All .wpt files in " + args.highwaydatapath +
          "/hwy_data processed.")
unprocessedfile.close()
all_wpt_files = None

# Near-miss point log
print(et.et() + "Near-miss point log and tm-master.nmp file.", flush=True)

# read in fp file
nmpfps = set()
nmpfpfile = open(args.highwaydatapath+'/nmpfps.log','r')
nmpfpfilelines = nmpfpfile.readlines()
for line in nmpfpfilelines:
    if len(line.rstrip('\n ')) > 0:
        nmpfps.add(line.rstrip('\n '))
nmpfpfile.close()

nmploglines = []
nmplog = open(args.logfilepath+'/nearmisspoints.log','w')
nmpnmp = open(args.logfilepath+'/tm-master.nmp','w')
for w in all_waypoints.point_list():
    if w.near_miss_points is not None:
        # sort the near miss points for consistent ordering to facilitate
        # NMP FP marking
        w.near_miss_points.sort(key=lambda waypoint:
                                waypoint.route.root + "@" + waypoint.label)
        # construct string for nearmisspoints.log & FP matching
        nmpline = str(w) + " NMP"
        for other_w in w.near_miss_points:
            nmpline += " " + str(other_w)
        # check for string in fp list
        fp = nmpline in nmpfps
        lifp_tag = 0
        if not fp:
            if nmpline+" [LOOKS INTENTIONAL]" in nmpfps:
                fp = True
                lifp_tag = 1
        if not fp:
            if nmpline+" [SOME LOOK INTENTIONAL]" in nmpfps:
                fp = True
                lifp_tag = 2
        #  write lines to tm-master.nmp
        li_count = 0
        for other_w in w.near_miss_points:
            li = (abs(w.lat - other_w.lat) < 0.0000015) and \
                 (abs(w.lng - other_w.lng) < 0.0000015)
            if li:
                li_count += 1
            w_label = w.route.root + "@" + w.label
            other_label = other_w.route.root + "@" + other_w.label
            # make sure we only plot once, since the NMP should be listed
            # both ways (other_w in w's list, w in other_w's list)
            if w_label < other_label:
                nmpnmp.write(w_label + " " + str(w.lat) + " " + str(w.lng))
                if fp or li:
                    nmpnmp.write(' ')
                    if fp:
                        nmpnmp.write('FP')
                    if li:
                        nmpnmp.write('LI')
                nmpnmp.write('\n')
                nmpnmp.write(other_label + " " + str(other_w.lat) + " " + str(other_w.lng))
                if fp or li:
                    nmpnmp.write(' ')
                    if fp:
                        nmpnmp.write('FP')
                    if li:
                        nmpnmp.write('LI')
                nmpnmp.write('\n')
        # indicate if this was in the FP list or if it's off by exact amt
        # so looks like it's intentional, and detach near_miss_points list
        # so it doesn't get a rewrite in nmp_merged WPT files
        logline = nmpline
        if li_count:
            if li_count == len(w.near_miss_points):
                logline += " [LOOKS INTENTIONAL]"
            else:
                logline += " [SOME LOOK INTENTIONAL]"
            w.near_miss_points = None
        if fp:
            if lifp_tag == 0:
                nmpfps.remove(nmpline)
            elif lifp_tag == 1:
                nmpfps.remove(nmpline+" [LOOKS INTENTIONAL]")
            else:
                nmpfps.remove(nmpline+" [SOME LOOK INTENTIONAL]")
            logline += " [MARKED FP]"
            w.near_miss_points = None
        nmploglines.append(logline)
nmpnmp.close()

# sort and write actual lines to nearmisspoints.log
nmploglines.sort()
for n in nmploglines:
    nmplog.write(n + '\n')
nmploglines = None
nmplog.close()

# report any unmatched nmpfps.log entries
nmpfpsunmatchedfile = open(args.logfilepath+'/nmpfpsunmatched.log','w')
for line in sorted(nmpfps):
    nmpfpsunmatchedfile.write(line + '\n')
nmpfpsunmatchedfile.close()
nmpfps = None

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

print(et.et() + "Processing waypoint labels and checking for unconnected chopped routes.",flush=True)
for h in highway_systems:
    for r in h.route_list:
        # check for unconnected chopped routes
        if r.con_route is None:
            el.add_error(r.system.systemname + ".csv: root " + r.root + " not matched by any connected route root.")
            continue

        # check for mismatched route endpoints within connected routes
        q = r.con_route.roots[r.rootOrder-1]
        if r.rootOrder > 0 and len(q.point_list) > 1 and len(r.point_list) > 1 and not r.con_beg().same_coords(q.con_end()):
            if q.con_beg().same_coords(r.con_beg()):
                q.is_reversed = True
            elif q.con_end().same_coords(r.con_end()):
                r.is_reversed = True
            elif q.con_beg().same_coords(r.con_end()):
                q.is_reversed = True
                r.is_reversed = True
            else:
                datacheckerrors.append(DatacheckEntry(r, [r.con_beg().label],
                                       "DISCONNECTED_ROUTE", q.root + '@' + q.con_end().label))
                datacheckerrors.append(DatacheckEntry(q, [q.con_end().label],
                                       "DISCONNECTED_ROUTE", r.root + '@' + r.con_beg().label))

        # create label hashes and check for duplicates
        index = 0
        for w in r.point_list:
            # ignore case and leading '+' or '*'
            upper_label = w.label.lstrip('+*').upper()
            # if primary label not duplicated, add to r.pri_label_hash
            if upper_label in r.alt_label_hash:
                datacheckerrors.append(DatacheckEntry(r, [w.label], "DUPLICATE_LABEL"))
                r.duplicate_labels.add(upper_label)
            elif upper_label in r.pri_label_hash:
                datacheckerrors.append(DatacheckEntry(r, [w.label], "DUPLICATE_LABEL"))
                r.duplicate_labels.add(upper_label)
            else:
                r.pri_label_hash[upper_label] = index
            for a in range(len(w.alt_labels)):
                # create canonical AltLabels
                w.alt_labels[a] = w.alt_labels[a].lstrip('+*').upper()
                # populate unused set
                r.unused_alt_labels.add(w.alt_labels[a])
                # create label->index hashes and check if AltLabels duplicated
                if w.alt_labels[a] in r.pri_label_hash:
                    datacheckerrors.append(DatacheckEntry(r, [r.point_list[r.pri_label_hash[w.alt_labels[a]]].label], "DUPLICATE_LABEL"))
                    r.duplicate_labels.add(w.alt_labels[a])
                elif w.alt_labels[a] in r.alt_label_hash:
                    datacheckerrors.append(DatacheckEntry(r, [w.alt_labels[a]], "DUPLICATE_LABEL"))
                    r.duplicate_labels.add(w.alt_labels[a])
                else:
                    r.alt_label_hash[w.alt_labels[a]] = index
            index += 1

# Read updates.csv file, just keep in the fields array for now since we're
# just going to drop this into the DB later anyway
updates = []
print(et.et() + "Reading updates file.",flush=True)
with open(args.highwaydatapath+"/updates.csv", "rt", encoding='UTF-8') as file:
    lines = file.readlines()
file.close()

lines.pop(0)  # ignore header line
for line in lines:
    line=line.strip()
    if len(line) == 0:
        continue
    fields = line.split(';')
    if len(fields) != 5:
        el.add_error("Could not parse updates.csv line: [" + line +
                     "], expected 5 fields, found " + str(len(fields)))
        continue
    # verify field lengths
    if len(fields[0].encode('utf-8')) > DBFieldLength.date:
        el.add_error("date > " + str(DBFieldLength.date) +
                     " bytes in updates.csv line " + line)
    if len(fields[1].encode('utf-8')) > DBFieldLength.countryRegion:
        el.add_error("region > " + str(DBFieldLength.countryRegion) +
                     " bytes in updates.csv line " + line)
    if len(fields[2].encode('utf-8')) > DBFieldLength.routeLongName:
        el.add_error("route > " + str(DBFieldLength.routeLongName) +
                     " bytes in updates.csv line " + line)
    if len(fields[3].encode('utf-8')) > DBFieldLength.root:
        el.add_error("root > " + str(DBFieldLength.root) +
                     " bytes in updates.csv line " + line)
    if len(fields[4].encode('utf-8')) > DBFieldLength.updateText:
        el.add_error("description > " + str(DBFieldLength.updateText) +
                     " bytes in updates.csv line " + line)
    updates.append(fields)
    try:
        r = Route.root_hash[fields[3]]
        if r.last_update is None or r.last_update[0] < fields[0]:
            r.last_update = fields;
    except KeyError:
        pass

# Same plan for systemupdates.csv file, again just keep in the fields
# array for now since we're just going to drop this into the DB later
# anyway
systemupdates = []
print(et.et() + "Reading systemupdates file.",flush=True)
with open(args.highwaydatapath+"/systemupdates.csv", "rt", encoding='UTF-8') as file:
    lines = file.readlines()
file.close()

lines.pop(0)  # ignore header line
for line in lines:
    line=line.strip()
    if len(line) == 0:
        continue
    fields = line.split(';')
    if len(fields) != 5:
        el.add_error("Could not parse systemupdates.csv line: [" + line +
                     "], expected 5 fields, found " + str(len(fields)))
        continue
    # verify field lengths
    if len(fields[0].encode('utf-8')) > DBFieldLength.date:
        el.add_error("date > " + str(DBFieldLength.date) +
                     " bytes in systemupdates.csv line " + line)
    if len(fields[1].encode('utf-8')) > DBFieldLength.countryRegion:
        el.add_error("region > " + str(DBFieldLength.countryRegion) +
                     " bytes in systemupdates.csv line " + line)
    if len(fields[2].encode('utf-8')) > DBFieldLength.systemName:
        el.add_error("systemName > " + str(DBFieldLength.systemName) +
                     " bytes in systemupdates.csv line " + line)
    if len(fields[3].encode('utf-8')) > DBFieldLength.systemFullName:
        el.add_error("description > " + str(DBFieldLength.systemFullName) +
                     " bytes in systemupdates.csv line " + line)
    if len(fields[4].encode('utf-8')) > DBFieldLength.statusChange:
        el.add_error("statusChange > " + str(DBFieldLength.statusChange) +
                     " bytes in systemupdates.csv line " + line)
    systemupdates.append(fields)

# Read most recent update dates/times for .list files
# one line for each traveler, containing 4 space-separated fields:
# 0: username with .list extension
# 1-3: date, time, and time zone as written by "git log -n 1 --pretty=%ci"
listupdates = {}
try:
    file = open("listupdates.txt", "rt", encoding='UTF-8')
    print(et.et() + "Reading .list updates file.",flush=True)
    for line in file.readlines():
        line = line.strip()
        fields = line.split(' ')
        if len(fields) != 4:
            print("WARNING: Could not parse listupdates.txt line: [" +
                  line + "], expected 4 fields, found " + str(len(fields)))
            continue
        listupdates[fields[0]] = fields[1:]
    file.close()
except FileNotFoundError:
    pass

# Create a list of TravelerList objects, one per person
traveler_lists = []

print(et.et() + "Processing traveler list files:",flush=True)
for t in traveler_ids:
    if t.endswith('.list'):
        try:
            update = listupdates[t]
        except KeyError:
            update = None
        print(t + " ",end="",flush=True)
        traveler_lists.append(TravelerList(t,update,el,args.userlistfilepath))
del traveler_ids
del listupdates
print('\n' + et.et() + "Processed " + str(len(traveler_lists)) + " traveler list files.")
traveler_lists.sort(key=lambda TravelerList: TravelerList.traveler_name)
# assign traveler numbers
travnum = 0
for t in traveler_lists:
    t.traveler_num = travnum
    travnum += 1

print(et.et() + "Clearing route & label hash tables.",flush=True)
del Route.root_hash
del Route.pri_list_hash
del Route.alt_list_hash
for h in highway_systems:
    for r in h.route_list:
        del r.pri_label_hash
        del r.alt_label_hash
        del r.duplicate_labels

print(et.et() + "Writing pointsinuse.log, unusedaltlabels.log, listnamesinuse.log and unusedaltroutenames.log",flush=True)
total_unused_alt_labels = 0
total_unusedaltroutenames = 0
unused_alt_labels = []
piufile = open(args.logfilepath+'/pointsinuse.log','w',encoding='UTF-8')
lniufile = open(args.logfilepath+'/listnamesinuse.log','w',encoding='UTF-8')
uarnfile = open(args.logfilepath+'/unusedaltroutenames.log','w',encoding='UTF-8')
piufile.write("Log file created at: " + str(datetime.datetime.now()) + "\n")
lniufile.write("Log file created at: " + str(datetime.datetime.now()) + "\n")
uarnfile.write("Log file created at: " + str(datetime.datetime.now()) + "\n")
for h in highway_systems:
    for r in h.route_list:
        # labelsinuse.log line
        if len(r.labels_in_use) > 0:
            piufile.write(r.root + "(" + str(len(r.point_list)) + "):")
            for label in sorted(r.labels_in_use):
                piufile.write(" " + label)
            piufile.write("\n")
        del r.labels_in_use
        # unusedaltlabels.log lines, to be sorted by root later
        if len(r.unused_alt_labels) > 0:
            total_unused_alt_labels += len(r.unused_alt_labels)
            ual_entry = r.root + "(" + str(len(r.unused_alt_labels)) + "):"
            for label in sorted(r.unused_alt_labels):
                ual_entry += " " + label
            unused_alt_labels.append(ual_entry)
        del r.unused_alt_labels
    #listnamesinuse.log line
    if len(h.listnamesinuse) > 0:
        lniufile.write(h.systemname + '(' + str(len(h.route_list)) + "):")
        for list_name in sorted(h.listnamesinuse):
            lniufile.write(" \"" + list_name + '"')
        lniufile.write('\n')
    del h.listnamesinuse
    # unusedaltroutenames.log line
    if len(h.unusedaltroutenames) > 0:
        total_unusedaltroutenames += len(h.unusedaltroutenames)
        uarnfile.write(h.systemname + '(' + str(len(h.unusedaltroutenames)) + "):")
        for list_name in sorted(h.unusedaltroutenames):
            uarnfile.write(" \"" + list_name + '"')
        uarnfile.write('\n')
    del h.unusedaltroutenames
piufile.close()
lniufile.close()
uarnfile.write("Total: " + str(total_unusedaltroutenames) + '\n')
uarnfile.close()
# sort lines and write unusedaltlabels.log
unused_alt_labels.sort()
ualfile = open(args.logfilepath+'/unusedaltlabels.log','w',encoding='UTF-8')
ualfile.write("Log file created at: " + str(datetime.datetime.now()) + "\n")
for ual_entry in unused_alt_labels:
    ualfile.write(ual_entry + "\n")
unused_alt_labels = None
ualfile.write("Total: " + str(total_unused_alt_labels) + "\n")
ualfile.close()


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
                if hs != s and hs.route.system.active_or_preview() and hs.add_clinched_by(t):
                    concurrencyfile.write("Concurrency augment for traveler " + t.traveler_name + ": [" + str(hs) + "] based on [" + str(s) + "]\n")
print("!")
concurrencyfile.close()

"""sanetravfile = open(args.logfilepath+'/concurrent_travelers_sanity_check.log','w',encoding='utf-8')
for h in highway_systems:
    for r in h.route_list:
        for s in r.segment_list:
            sanetravfile.write(s.concurrent_travelers_sanity_check())
sanetravfile.close()
"""

# compute lots of regional stats:
# overall, active+preview, active only,
# and per-system which falls into just one of these categories
print(et.et() + "Computing stats.",end="",flush=True)
active_only_mileage_by_region = dict()
active_preview_mileage_by_region = dict()
overall_mileage_by_region = dict()
for h in highway_systems:
    print(".",end="",flush=True)
    for r in h.route_list:
        for s in r.segment_list:
            # always add the segment mileage to the route
            r.mileage += s.length
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
                    s.length/overall_concurrency_count
            else:
                overall_mileage_by_region[r.region] = s.length/overall_concurrency_count

            # next, same thing for active_preview mileage for the region,
            # if active or preview
            if r.system.active_or_preview():
                if r.region in active_preview_mileage_by_region:
                    active_preview_mileage_by_region[r.region] = active_preview_mileage_by_region[r.region] + \
                    s.length/active_preview_concurrency_count
                else:
                    active_preview_mileage_by_region[r.region] = s.length/active_preview_concurrency_count

            # now same thing for active_only mileage for the region,
            # if active
            if r.system.active():
                if r.region in active_only_mileage_by_region:
                    active_only_mileage_by_region[r.region] = active_only_mileage_by_region[r.region] + \
                    s.length/active_only_concurrency_count
                else:
                    active_only_mileage_by_region[r.region] = s.length/active_only_concurrency_count

            # now we move on to totals by region, only the
            # overall since an entire highway system must be
            # at the same level
            if r.region in h.mileage_by_region:
                h.mileage_by_region[r.region] = h.mileage_by_region[r.region] + \
                        s.length/system_concurrency_count
            else:
                h.mileage_by_region[r.region] = s.length/system_concurrency_count

            # that's it for overall stats, now credit all travelers
            # who have clinched this segment in their stats
            for t in s.clinched_by:
                # credit active+preview for this region, which it must be
                # if this segment is clinched by anyone
                if r.region in t.active_preview_mileage_by_region:
                    t.active_preview_mileage_by_region[r.region] = t.active_preview_mileage_by_region[r.region] + \
                        s.length/active_preview_concurrency_count
                else:
                    t.active_preview_mileage_by_region[r.region] = s.length/active_preview_concurrency_count

                # credit active only for this region
                if r.system.active():
                    if r.region in t.active_only_mileage_by_region:
                        t.active_only_mileage_by_region[r.region] = t.active_only_mileage_by_region[r.region] + \
                            s.length/active_only_concurrency_count
                    else:
                        t.active_only_mileage_by_region[r.region] = s.length/active_only_concurrency_count


                # credit this system in this region in the messy dictionary of dictionaries
                if h.systemname not in t.system_region_mileages:
                    t.system_region_mileages[h.systemname] = dict()
                t_system_dict = t.system_region_mileages[h.systemname]
                if r.region in t_system_dict:
                    t_system_dict[r.region] = t_system_dict[r.region] + \
                    s.length/system_concurrency_count
                else:
                    t_system_dict[r.region] = s.length/system_concurrency_count
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
        to_write = ""
        for r in cr.roots:
            to_write += "  " + r.readable_name() + ": " + "{0:.2f}".format(r.mileage) + " mi\n"
            cr.mileage += r.mileage
        hdstatsfile.write(cr.readable_name() + ": " + "{0:.2f}".format(cr.mileage) + " mi")
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
                t.log_entries.append("System " + h.systemname + " (" + h.level + ") overall: " +
                                     format_clinched_mi(t_system_overall, math.fsum(list(h.mileage_by_region.values()))))
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
                con_routes_traveled = 0
                con_routes_clinched = 0
                t.log_entries.append("System " + h.systemname + " by route (traveled routes only):")
                for cr in h.con_route_list:
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
                            con_clinched_miles += miles
                            to_write += "  " + r.readable_name() + ": " + \
                                format_clinched_mi(miles,r.mileage) + "\n"
                    if con_clinched_miles > 0:
                        con_routes_traveled += 1
                        clinched = '0'
                        if con_clinched_miles == cr.mileage:
                            con_routes_clinched += 1
                            clinched = '1'
                        ccr_values.append("('" + cr.roots[0].root + "','" + t.traveler_name
                                          + "','" + str(con_clinched_miles) + "','"
                                          + clinched + "')")
                        t.log_entries.append(cr.readable_name() + ": " + \
                                             format_clinched_mi(con_clinched_miles,cr.mileage))
                        if len(cr.roots) == 1:
                            t.log_entries.append(" (" + cr.roots[0].readable_name() + " only)")
                        else:
                            t.log_entries.append(to_write)
                t.log_entries.append("System " + h.systemname + " connected routes traveled: " + \
                                         str(con_routes_traveled) + " of " + \
                                         str(len(h.con_route_list)) + \
                                         " ({0:.1f}%)".format(100*con_routes_traveled/len(h.con_route_list)) + \
                                         ", clinched: " + str(con_routes_clinched) + " of " + \
                                         str(len(h.con_route_list)) + \
                                         " ({0:.1f}%)".format(100*con_routes_clinched/len(h.con_route_list)) + \
                                         ".")


    # grand summary, active only
    t.log_entries.append("\nTraveled " + str(t.active_systems_traveled) + " of " + str(active_systems) +
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
    # updated routes, sorted by date
    t.log_entries.append("\nMost recent updates for listed routes:")
    route_list = []
    for r in t.routes:
        if r.last_update:
            route_list.append(r)
    del t.routes
    route_list.sort(key=lambda r: r.last_update[0]+r.last_update[3])
    for r in route_list:
        t.log_entries.append(r.last_update[0] + " | " + r.last_update[1] + " | " + \
                             r.last_update[2] + " | " + r.last_update[3] + " | " + r.last_update[4])

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
datacheck_always_error = [ 'BAD_ANGLE', 'DISCONNECTED_ROUTE', 'DUPLICATE_LABEL', 
                           'HIDDEN_TERMINUS', 'INTERSTATE_NO_HYPHEN',
                           'INVALID_FINAL_CHAR', 'INVALID_FIRST_CHAR',
                           'LABEL_INVALID_CHAR', 'LABEL_PARENS', 'LABEL_SLASHES',
                           'LABEL_TOO_LONG', 'LABEL_UNDERSCORES', 'LONG_UNDERSCORE',
                           'MALFORMED_LAT', 'MALFORMED_LON', 'MALFORMED_URL',
                           'NONTERMINAL_UNDERSCORE', 'US_LETTER' ]
for line in lines:
    line=line.strip()
    if len(line) == 0:
        continue
    fields = line.split(';')
    if len(fields) != 6:
        el.add_error("Could not parse datacheckfps.csv line: [" + line +
                     "], expected 6 fields, found " + str(len(fields)))
        continue
    if fields[4] in datacheck_always_error:
        print("datacheckfps.csv line not allowed (always error): " + line)
        continue
    datacheckfps.append(fields)

# Build a graph structure out of all highway data in active and
# preview systems
print(et.et() + "Setting up for graphs of highway data.", flush=True)
graph_data = HighwayGraph(all_waypoints, highway_systems, datacheckerrors, et)

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
    print(et.et() + "Writing master TM graph files.", flush=True)
    graph_data.write_master_graphs_tmg(graph_list, args.graphfilepath+'/', traveler_lists)
    graph_types.append(['master', 'All Travel Mapping Data',
                        'These graphs contain all routes currently plotted in the Travel Mapping project.'])



    # graphs restricted by place/area - from areagraphs.csv file
    print(et.et() + "Creating area data graphs.", flush=True)
    with open(args.highwaydatapath+"/graphs/areagraphs.csv", "rt",encoding='utf-8') as file:
        lines = file.readlines()
    file.close()
    lines.pop(0);  # ignore header line
    area_list = []
    for line in lines:
        line=line.strip()
        if len(line) == 0:
            continue
        fields = line.split(";")
        if len(fields) != 5:
            el.add_error("Could not parse areagraphs.csv line: [" + line +
                         "], expected 5 fields, found " + str(len(fields)))
            continue
        if len(fields[0].encode('utf-8'))+len(fields[4]) > DBFieldLength.graphDescr-13:
            el.add_error("description + radius is too long by " +
                         str(len(fields[0].encode('utf-8'))+len(fields[4]) - DBFieldLength.graphDescr+13) +
                         " byte(s) in areagraphs.csv line: " + line)
        if len(fields[1].encode('utf-8'))+len(fields[4]) > DBFieldLength.graphFilename-19:
            el.add_error("title + radius = filename too long by " +
                         str(len(fields[1].encode('utf-8'))+len(fields[4]) - DBFieldLength.graphFilename+19) +
                         " byte(s) in areagraphs.csv line: " + line)
        # convert numeric fields
        try:
            fields[2] = float(fields[2])
        except ValueError:
            el.add_error("invalid lat in areagraphs.csv line: " + line)
            fields[2] = 0.0
        try:
            fields[3] = float(fields[3])
        except ValueError:
            el.add_error("invalid lng in areagraphs.csv line: " + line)
            fields[3] = 0.0
        try:
            fields[4] = int(fields[4])
        except ValueError:
            el.add_error("invalid radius in areagraphs.csv line: " + line)
            fields[4] = 1
        else:
            if fields[4] <= 0:
                el.add_error("invalid radius in areagraphs.csv line: " + line)
                fields[4] = 1
        area_list.append(PlaceRadius(*fields))

    for a in area_list:
        print(a.title + '(' + str(a.r) + ') ', end="", flush=True)
        graph_data.write_subgraphs_tmg(graph_list, args.graphfilepath + "/", a.title + str(a.r) + "-area",
                                       a.descr + " (" + str(a.r) + " mi radius)", "area",
                                       None, None, a, all_waypoints)
    graph_types.append(['area', 'Routes Within a Given Radius of a Place',
                        'These graphs contain all routes currently plotted within the given distance radius of the given place.'])
    print("!")
        


    # Graphs restricted by region
    print(et.et() + "Creating regional data graphs.", flush=True)

    # We will create graph data and a graph file for each region that includes
    # any active or preview systems
    for r in all_regions.values():
        region_code = r[0]
        if region_code not in active_preview_mileage_by_region:
            continue
        region_name = r[1]
        region_type = r[4]
        print(region_code + ' ', end="",flush=True)
        graph_data.write_subgraphs_tmg(graph_list, args.graphfilepath + "/", region_code + "-region",
                                       region_name + " (" + region_type + ")", "region",
                                       [ region_code ], None, None, all_waypoints)
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
                                           h.systemname + " (" + h.fullname + ")", "system",
                                           None, [ h ], None, all_waypoints)
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
        line=line.strip()
        if len(line) == 0:
            continue
        fields = line.split(";")
        if len(fields) != 3:
            el.add_error("Could not parse multisystem.csv line: [" + line +
                         "], expected 3 fields, found " + str(len(fields)))
            continue
        if len(fields[0].encode('utf-8')) > DBFieldLength.graphDescr:
            el.add_error("description > " + str(DBFieldLength.graphDescr) +
                         " bytes in multisystem.csv line: " + line)
        if len(fields[1].encode('utf-8')) > DBFieldLength.graphFilename-14:
            el.add_error("title > " + str(DBFieldLength.graphFilename-14) +
                         " bytes in multisystem.csv line: " + line)
        print(fields[1] + ' ', end="", flush=True)
        systems = []
        selected_systems = fields[2].split(",")
        for h in highway_systems:
            if h.systemname in selected_systems:
                systems.append(h)
        graph_data.write_subgraphs_tmg(graph_list, args.graphfilepath + "/", fields[1],
                                       fields[0], "multisystem",
                                       None, systems, None, all_waypoints)
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
        line=line.strip()
        if len(line) == 0:
            continue
        fields = line.split(";")
        if len(fields) != 3:
            el.add_error("Could not parse multiregion.csv line: [" + line +
                         "], expected 3 fields, found " + str(len(fields)))
            continue
        if len(fields[0].encode('utf-8')) > DBFieldLength.graphDescr:
            el.add_error("description > " + str(DBFieldLength.graphDescr) +
                         " bytes in multiregion.csv line: " + line)
        if len(fields[1].encode('utf-8')) > DBFieldLength.graphFilename-14:
            el.add_error("title > " + str(DBFieldLength.graphFilename-14) +
                         " bytes in multiregion.csv line: " + line)
        print(fields[1] + ' ', end="", flush=True)
        region_list = []
        selected_regions = fields[2].split(",")
        for r in all_regions.keys():
            if r in selected_regions and r in active_preview_mileage_by_region:
                region_list.append(r)
        graph_data.write_subgraphs_tmg(graph_list, args.graphfilepath + "/", fields[1],
                                       fields[0], "multiregion",
                                       region_list, None, None, all_waypoints)
    graph_types.append(['multiregion', 'Routes Within Multiple Regions',
                        'These graphs contain the routes within a set of regions.'])
    print("!")



    # country graphs - we find countries that have regions
    # that have routes with active or preview mileage
    print(et.et() + "Creating country graphs.", flush=True)
    for c in countries:
        region_list = []
        for r in all_regions.values():
            # does it match this country and have routes?
            if c[0] == r[2] and r[0] in active_preview_mileage_by_region:
                region_list.append(r[0])
        # does it have at least two?  if none, no data, if 1 we already
        # generated a graph for that one region
        if len(region_list) >= 2:
            print(c[0] + " ", end="", flush=True)
            graph_data.write_subgraphs_tmg(graph_list, args.graphfilepath + "/", c[0] + "-country",
                                           c[1] + " All Routes in Country", "country",
                                           region_list, None, None, all_waypoints)
    graph_types.append(['country', 'Routes Within a Single Multi-Region Country',
                        'These graphs contain the routes within a single country that is composed of multiple regions that contain plotted routes.  Countries consisting of a single region are represented by their regional graph.'])
    print("!")



    # continent graphs -- any continent with data will be created
    print(et.et() + "Creating continent graphs.", flush=True)
    for c in continents:
        region_list = []
        for r in all_regions.values():
            # does it match this continent and have routes?
            if c[0] == r[3] and r[0] in active_preview_mileage_by_region:
                region_list.append(r[0])
        # generate for any continent with at least 1 region with mileage
        if len(region_list) >= 1:
            print(c[0] + " ", end="", flush=True)
            graph_data.write_subgraphs_tmg(graph_list, args.graphfilepath + "/", c[0] + "-continent",
                                           c[1] + " All Routes on Continent", "continent",
                                           region_list, None, None, all_waypoints)
    graph_types.append(['continent', 'Routes Within a Continent',
                        'These graphs contain the routes on a continent.'])
    print("!")

# data check: visit each system and route and check for various problems
print(et.et() + "Performing data checks.",end="",flush=True)
# perform most datachecks here (list initialized above)
for h in highway_systems:
    print(".",end="",flush=True)
    usa_flag = h.country == "USA"
    for r in h.route_list:
        # set of tuples to be used for finding duplicate coordinates
        coords_used = set()

        visible_distance = 0.0
        # note that we assume the first point will be visible in each route
        # so the following is simply a placeholder
        last_visible = None
        prev_w = None

        # look for hidden termini
        if len(r.point_list) > 1:
            if r.point_list[0].is_hidden:
                datacheckerrors.append(DatacheckEntry(r,[r.point_list[0].label],'HIDDEN_TERMINUS'))
            if r.point_list[-1].is_hidden:
                datacheckerrors.append(DatacheckEntry(r,[r.point_list[-1].label],'HIDDEN_TERMINUS'))

        for w in r.point_list:
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
                    if w.lat == other_w.lat and w.lng == other_w.lng:
                        labels = []
                        labels.append(other_w.label)
                        labels.append(w.label)
                        datacheckerrors.append(DatacheckEntry(r,labels,"DUPLICATE_COORDS",
                                                              "("+str(latlng[0])+","+str(latlng[1])+")"))
            else:
               coords_used.add(latlng)

            # look for labels with invalid characters
            if not re.fullmatch('\+?\*?[a-zA-Z0-9()/_\-\.]+', w.label):
                if w.label.encode('utf-8')[0:3] == b'\xef\xbb\xbf':
                    datacheckerrors.append(DatacheckEntry(r,[w.label],'LABEL_INVALID_CHAR', "UTF-8 BOM"))
                else:
                    datacheckerrors.append(DatacheckEntry(r,[w.label],'LABEL_INVALID_CHAR'))
            for a in w.alt_labels:
                if not re.fullmatch('\+?\*?[a-zA-Z0-9()/_\-\.]+', a):
                    datacheckerrors.append(DatacheckEntry(r,[a],'LABEL_INVALID_CHAR'))

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
                # partially complete "references own route" -- too many FP
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
                    if w.label.index('_') < len(w.label) - 4:
                        if w.label[-1] not in string.ascii_uppercase or w.label.index('_') < len(w.label) - 5:
                            datacheckerrors.append(DatacheckEntry(r,[w.label],'LONG_UNDERSCORE'))

                # look for too many slashes in label
                if w.label.count('/') > 1:
                    datacheckerrors.append(DatacheckEntry(r,[w.label],'LABEL_SLASHES'))

                # check parentheses: only 0 of both or 1 of both, '(' before ')'
                left_count = w.label.count('(')
                right_count = w.label.count(')')
                if left_count != right_count \
                or left_count > 1 \
                or (left_count == 1 and w.label.index('(') > w.label.index(')')):
                    datacheckerrors.append(DatacheckEntry(r,[w.label],'LABEL_PARENS'))

                # look for labels with invalid first or last character
                index = 0
                while index < len(w.label) and w.label[index] == '*':
                    index += 1
                if index < len(w.label) and w.label[index] in "_/(":
                    datacheckerrors.append(DatacheckEntry(r,[w.label],'INVALID_FIRST_CHAR', w.label[index]))
                if w.label[-1] == "_" or w.label[-1] == "/":
                    datacheckerrors.append(DatacheckEntry(r,[w.label],'INVALID_FINAL_CHAR', w.label[-1]))

                # look for labels with a slash after an underscore
                if '_' in w.label and '/' in w.label and \
                        w.label.index('/') > w.label.index('_'):
                    datacheckerrors.append(DatacheckEntry(r,[w.label],'NONTERMINAL_UNDERSCORE'))

                # look for labels that look like hidden waypoints but
                # which aren't hidden
                if re.fullmatch('X[0-9][0-9][0-9][0-9][0-9][0-9]', w.label):
                    datacheckerrors.append(DatacheckEntry(r,[w.label],'LABEL_LOOKS_HIDDEN'))

                # lacks generic highway type
                if re.fullmatch('^\*?[Oo][lL][dD][0-9].*', w.label):
                    datacheckerrors.append(DatacheckEntry(r,[w.label],'LACKS_GENERIC'))

                # USA-only datachecks
                if usa_flag and len(w.label) >= 2:

                    # look for I-xx with Bus instead of BL or BS
                    if re.fullmatch('\*?I\-[0-9]+[EeWwCcNnSs]?[Bb][Uu][Ss].*', w.label):
                        datacheckerrors.append(DatacheckEntry(r,[w.label],'BUS_WITH_I'))

                    # look for Ixx without hyphen
                    c = 1 if w.label[0] == '*' else 0
                    if w.label[c:c+2] == "To":
                        c += 2;
                    if len(w.label) >= c+2 and w.label[c] == 'I' and w.label[c+1].isdigit():
                        datacheckerrors.append(DatacheckEntry(r,[w.label],'INTERSTATE_NO_HYPHEN'))

                    # look for USxxxA but not USxxxAlt, B/Bus/Byp
                    # Eric's paraphrase of Jim's original criteria
                    # if re.fullmatch('\*?US[0-9]+[AB].*', w.label) and not re.fullmatch('\*?US[0-9]+Alt.*|\*?US[0-9]+Bus.*|\*?US[0-9]+Byp.*', w.label):
                    # Instead, let's cast a narrower net (optimized for speed, just because):
                    try:
                        c = 3 if w.label[0] == '*' else 2
                        if w.label[c-2] == 'U' and w.label[c-1] == 'S' and w.label[c].isdigit():
                            while w.label[c].isdigit():
                                c += 1
                            if w.label[c] in 'AB':
                                c += 1
                                if c == len(w.label) or w.label[c] in '/_(':
                                    datacheckerrors.append(DatacheckEntry(r,[w.label],'US_LETTER'))
                                # is it followed by a city abbrev?
                                elif w.label[c].isupper() and w.label[c+1].islower() and w.label[c+2].islower() \
                                and (c+3 == len(w.label) or w.label[c+3] in '/_('):
                                    datacheckerrors.append(DatacheckEntry(r,[w.label],'US_LETTER'))
                    except IndexError:
                        pass
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

datacheckerrors.sort(key=lambda DatacheckEntry: str(DatacheckEntry))

# now mark false positives
print(et.et() + "Marking datacheck false positives.",end="",flush=True)
fpfile = open(args.logfilepath+'/nearmatchfps.log','w',encoding='utf-8')
fpfile.write("Log file created at: " + str(datetime.datetime.now()) + "\n")
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
print(et.et() + "Found " + str(len(datacheckerrors)) + " datacheck errors and matched " + str(fpcount) + " FP entries.", flush=True)

# write log of unmatched false positives from the datacheckfps.csv
print(et.et() + "Writing log of unmatched datacheck FP entries.", flush=True)
fpfile = open(args.logfilepath+'/unmatchedfps.log','w',encoding='utf-8')
fpfile.write("Log file created at: " + str(datetime.datetime.now()) + "\n")
if len(datacheckfps) > 0:
    for entry in datacheckfps:
        fpfile.write(entry[0] + ';' + entry[1] + ';' + entry[2] + ';' + entry[3] + ';' + entry[4] + ';' + entry[5] + '\n')
else:
    fpfile.write("No unmatched FP entries.")
fpfile.close()

# datacheck.log file
print(et.et() + "Writing datacheck.log", flush=True)
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
    
# See if we have any errors that should be fatal to the site update process
if len(el.error_list) > 0:
    print(et.et() + "ABORTING due to " + str(len(el.error_list)) + " errors:")
    for i in range(len(el.error_list)):
        print(str(i+1) + ": " + el.error_list[i])
    sys.exit(1)

if args.errorcheck:
    print(et.et() + "SKIPPING database file.", flush=True)
else:
    print(et.et() + "Writing database file " + args.databasename + ".sql.", flush=True)
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
    print(et.et() + "...continents, countries, regions", flush=True)
    sqlfile.write('CREATE TABLE continents (code VARCHAR(' + str(DBFieldLength.continentCode) +
                  '), name VARCHAR(' + str(DBFieldLength.continentName) +
                  '), PRIMARY KEY(code));\n')
    sqlfile.write('INSERT INTO continents VALUES\n')
    first = True
    for c in continents:
        if not first:
            sqlfile.write(",")
        first = False
        sqlfile.write("('" + c[0] + "','" + c[1] + "')\n")
    sqlfile.write(";\n")

    sqlfile.write('CREATE TABLE countries (code VARCHAR(' + str(DBFieldLength.countryCode) +
                  '), name VARCHAR(' + str(DBFieldLength.countryName) +
                  '), PRIMARY KEY(code));\n')
    sqlfile.write('INSERT INTO countries VALUES\n')
    first = True
    for c in countries:
        if not first:
            sqlfile.write(",")
        first = False
        sqlfile.write("('" + c[0] + "','" + c[1].replace("'","''") + "')\n")
    sqlfile.write(";\n")

    sqlfile.write('CREATE TABLE regions (code VARCHAR(' + str(DBFieldLength.regionCode) +
                  '), name VARCHAR(' + str(DBFieldLength.regionName) +
                  '), country VARCHAR(' + str(DBFieldLength.countryCode) +
                  '), continent VARCHAR(' + str(DBFieldLength.continentCode) +
                  '), regiontype VARCHAR(' + str(DBFieldLength.regiontype) +
                  '), PRIMARY KEY(code), FOREIGN KEY (country) REFERENCES countries(code), FOREIGN KEY (continent) REFERENCES continents(code));\n')
    sqlfile.write('INSERT INTO regions VALUES\n')
    first = True
    for r in all_regions.values():
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
    print(et.et() + "...systems", flush=True)
    sqlfile.write('CREATE TABLE systems (systemName VARCHAR(' + str(DBFieldLength.systemName) +
                  '), countryCode CHAR(' + str(DBFieldLength.countryCode) +
                  '), fullName VARCHAR(' + str(DBFieldLength.systemFullName) +
                  '), color VARCHAR(' + str(DBFieldLength.color) +
                  '), level VARCHAR(' + str(DBFieldLength.level) +
                  '), tier INTEGER, csvOrder INTEGER, PRIMARY KEY(systemName));\n')
    sqlfile.write('INSERT INTO systems VALUES\n')
    first = True
    csvOrder = 0
    for h in highway_systems:
        if not first:
            sqlfile.write(",")
        first = False
        sqlfile.write("('" + h.systemname + "','" +  h.country + "','" +
                      h.fullname.replace("'","''") + "','" + h.color + "','" + h.level +
                      "','" + str(h.tier) + "','" + str(csvOrder) + "')\n")
        csvOrder += 1
    sqlfile.write(";\n")

    # next, a table of highways, with the same fields as in the first line
    print(et.et() + "...routes", flush=True)
    sqlfile.write('CREATE TABLE routes (systemName VARCHAR(' + str(DBFieldLength.systemName) +
                  '), region VARCHAR(' + str(DBFieldLength.regionCode) +
                  '), route VARCHAR(' + str(DBFieldLength.route) +
                  '), banner VARCHAR(' + str(DBFieldLength.banner) +
                  '), abbrev VARCHAR(' + str(DBFieldLength.abbrev) +
                  '), city VARCHAR(' + str(DBFieldLength.city) +
                  '), root VARCHAR(' + str(DBFieldLength.root) +
                  '), mileage FLOAT, rootOrder INTEGER, csvOrder INTEGER, PRIMARY KEY(root), FOREIGN KEY (systemName) REFERENCES systems(systemName));\n')
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
    print(et.et() + "...connectedRoutes", flush=True)
    sqlfile.write('CREATE TABLE connectedRoutes (systemName VARCHAR(' + str(DBFieldLength.systemName) +
                  '), route VARCHAR(' + str(DBFieldLength.route) +
                  '), banner VARCHAR(' + str(DBFieldLength.banner) +
                  '), groupName VARCHAR(' + str(DBFieldLength.city) +
                  '), firstRoot VARCHAR(' + str(DBFieldLength.root) +
                  '), mileage FLOAT, csvOrder INTEGER, PRIMARY KEY(firstRoot), FOREIGN KEY (firstRoot) REFERENCES routes(root));\n')
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
    print(et.et() + "...connectedRouteRoots", flush=True)
    sqlfile.write('CREATE TABLE connectedRouteRoots (firstRoot VARCHAR(' + str(DBFieldLength.root) +
                  '), root VARCHAR(' + str(DBFieldLength.root) +
                  '), FOREIGN KEY (firstRoot) REFERENCES connectedRoutes(firstRoot));\n')
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
    print(et.et() + "...waypoints", flush=True)
    sqlfile.write('CREATE TABLE waypoints (pointId INTEGER, pointName VARCHAR(' + str(DBFieldLength.label) +
                  '), latitude DOUBLE, longitude DOUBLE, root VARCHAR(' + str(DBFieldLength.root) +
                  '), PRIMARY KEY(pointId), FOREIGN KEY (root) REFERENCES routes(root));\n')
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
    print(et.et() + "...segments", flush=True)
    sqlfile.write('CREATE TABLE segments (segmentId INTEGER, waypoint1 INTEGER, waypoint2 INTEGER, root VARCHAR(' + str(DBFieldLength.root) +
                  '), PRIMARY KEY (segmentId), FOREIGN KEY (waypoint1) REFERENCES waypoints(pointId), ' +
                  'FOREIGN KEY (waypoint2) REFERENCES waypoints(pointId), FOREIGN KEY (root) REFERENCES routes(root));\n')
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
    print(et.et() + "...clinched", flush=True)
    sqlfile.write('CREATE TABLE clinched (segmentId INTEGER, traveler VARCHAR(' + str(DBFieldLength.traveler) +
                  '), FOREIGN KEY (segmentId) REFERENCES segments(segmentId));\n')
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
    print(et.et() + "...overallMileageByRegion", flush=True)
    sqlfile.write('CREATE TABLE overallMileageByRegion (region VARCHAR(' + str(DBFieldLength.regionCode) +
                  '), activeMileage FLOAT, activePreviewMileage FLOAT);\n')
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
    print(et.et() + "...systemMileageByRegion", flush=True)
    sqlfile.write('CREATE TABLE systemMileageByRegion (systemName VARCHAR(' + str(DBFieldLength.systemName) +
                  '), region VARCHAR(' + str(DBFieldLength.regionCode) +
                  '), mileage FLOAT, FOREIGN KEY (systemName) REFERENCES systems(systemName));\n')
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
    print(et.et() + "...clinchedOverallMileageByRegion", flush=True)
    sqlfile.write('CREATE TABLE clinchedOverallMileageByRegion (region VARCHAR(' + str(DBFieldLength.regionCode) +
                  '), traveler VARCHAR(' + str(DBFieldLength.traveler) +
                  '), activeMileage FLOAT, activePreviewMileage FLOAT);\n')
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
    print(et.et() + "...clinchedSystemMileageByRegion", flush=True)
    sqlfile.write('CREATE TABLE clinchedSystemMileageByRegion (systemName VARCHAR(' + str(DBFieldLength.systemName) +
                  '), region VARCHAR(' + str(DBFieldLength.regionCode) +
                  '), traveler VARCHAR(' + str(DBFieldLength.traveler) +
                  '), mileage FLOAT, FOREIGN KEY (systemName) REFERENCES systems(systemName));\n')
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
    print(et.et() + "...clinchedConnectedRoutes", flush=True)
    sqlfile.write('CREATE TABLE clinchedConnectedRoutes (route VARCHAR(' + str(DBFieldLength.root) +
                  '), traveler VARCHAR(' + str(DBFieldLength.traveler) +
                  '), mileage FLOAT, clinched BOOLEAN, FOREIGN KEY (route) REFERENCES connectedRoutes(firstRoot));\n')
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
    print(et.et() + "...clinchedRoutes", flush=True)
    sqlfile.write('CREATE TABLE clinchedRoutes (route VARCHAR(' + str(DBFieldLength.root) +
                  '), traveler VARCHAR(' + str(DBFieldLength.traveler) +
                  '), mileage FLOAT, clinched BOOLEAN, FOREIGN KEY (route) REFERENCES routes(root));\n')
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
    print(et.et() + "...updates", flush=True)
    sqlfile.write('CREATE TABLE updates (date VARCHAR(' + str(DBFieldLength.date) +
                  '), region VARCHAR(' + str(DBFieldLength.countryRegion) +
                  '), route VARCHAR(' + str(DBFieldLength.routeLongName) +
                  '), root VARCHAR(' + str(DBFieldLength.root) +
                  '), description VARCHAR(' + str(DBFieldLength.updateText) +
                  '));\n')
    sqlfile.write('INSERT INTO updates VALUES\n')
    first = True
    for update in updates:
        if not first:
            sqlfile.write(",")
        first = False
        sqlfile.write("('"+update[0]+"','"+update[1].replace("'","''")+"','"+update[2].replace("'","''")+"','"+update[3]+"','"+update[4].replace("'","''")+"')\n")
    sqlfile.write(";\n")

    # systemUpdates entries
    print(et.et() + "...systemUpdates", flush=True)
    sqlfile.write('CREATE TABLE systemUpdates (date VARCHAR(' + str(DBFieldLength.date) +
                  '), region VARCHAR(' + str(DBFieldLength.countryRegion) +
                  '), systemName VARCHAR(' + str(DBFieldLength.systemName) +
                  '), description VARCHAR(' + str(DBFieldLength.systemFullName) +
                  '), statusChange VARCHAR(' + str(DBFieldLength.statusChange) +
                  '));\n')
    sqlfile.write('INSERT INTO systemUpdates VALUES\n')
    first = True
    for systemupdate in systemupdates:
        if not first:
            sqlfile.write(",")
        first = False
        sqlfile.write("('"+systemupdate[0]+"','"+systemupdate[1].replace("'","''")+"','"+systemupdate[2]+"','"+systemupdate[3].replace("'","''")+"','"+systemupdate[4]+"')\n")
    sqlfile.write(";\n")

    # datacheck errors into the db
    print(et.et() + "...datacheckErrors", flush=True)
    sqlfile.write('CREATE TABLE datacheckErrors (route VARCHAR(' + str(DBFieldLength.root) +
                  '), label1 VARCHAR(' + str(DBFieldLength.label) +
                  '), label2 VARCHAR(' + str(DBFieldLength.label) +
                  '), label3 VARCHAR(' + str(DBFieldLength.label) +
                  '), code VARCHAR(' + str(DBFieldLength.dcErrCode) +
                  '), value VARCHAR(' + str(DBFieldLength.dcErrValue) +
                  '), falsePositive BOOLEAN, FOREIGN KEY (route) REFERENCES routes(root));\n')
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
        print(et.et() + "...graphs", flush=True)
        sqlfile.write('DROP TABLE IF EXISTS graphs;\n')
        sqlfile.write('DROP TABLE IF EXISTS graphTypes;\n')
        sqlfile.write('CREATE TABLE graphTypes (category VARCHAR(' + str(DBFieldLength.graphCategory) +
                  '), descr VARCHAR(' + str(DBFieldLength.graphDescr) +
                  '), longDescr TEXT, PRIMARY KEY(category));\n')
        if len(graph_types) > 0:
            sqlfile.write('INSERT INTO graphTypes VALUES\n')
            first = True
            for g in graph_types:
                if not first:
                    sqlfile.write(',')
                first = False
                sqlfile.write("('" + g[0] + "','" + g[1] + "','" + g[2] + "')\n")
            sqlfile.write(";\n")

        sqlfile.write('CREATE TABLE graphs (filename VARCHAR(' + str(DBFieldLength.graphFilename) +
                  '), descr VARCHAR(' + str(DBFieldLength.graphDescr) +
                  '), vertices INTEGER, edges INTEGER, travelers INTEGER, format VARCHAR(' + str(DBFieldLength.graphFormat) +
                  '), category VARCHAR(' + str(DBFieldLength.graphCategory) +
                  '), FOREIGN KEY (category) REFERENCES graphTypes(category));\n')
        if len(graph_list) > 0:
            sqlfile.write('INSERT INTO graphs VALUES\n')
            first = True
            for g in graph_list:
                if not first:
                    sqlfile.write(',')
                first = False
                sqlfile.write("('" + g.filename + "','" + g.descr.replace("'","''") + "','" + str(g.vertices) + "','" + str(g.edges) + "','" + str(g.travelers) + "','" + g.format + "','" + g.category + "')\n")
            sqlfile.write(";\n")


    sqlfile.close()

# print some statistics
print(et.et() + "Processed " + str(len(highway_systems)) + " highway systems.", flush=True)
routes = 0
points = 0
segments = 0
for h in highway_systems:
    routes += len(h.route_list)
    for r in h.route_list:
        points += len(r.point_list)
        segments += len(r.segment_list)
print("Processed " + str(routes) + " routes with a total of " + \
          str(points) + " points and " + str(segments) + " segments.", flush=True)
if points != all_waypoints.size():
    print("MISMATCH: all_waypoints contains " + str(all_waypoints.size()) + " waypoints!", flush=True)
print("WaypointQuadtree contains " + str(all_waypoints.total_nodes()) + " total nodes.", flush=True)

if not args.errorcheck:
    # compute colocation of waypoints stats
    print(et.et() + "Computing waypoint colocation stats, reporting all with 8 or more colocations:", flush=True)
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
            #print(str(w) + " with " + str(c) + " other points.", flush=True)
        colocate_counts[c] += 1
    for place in big_colocate_locations:
        the_list = big_colocate_locations[place]
        print(str(place) + " is occupied by " + str(len(the_list)) + " waypoints: " + str(the_list), flush=True)
    print("Waypoint colocation counts:", flush=True)
    unique_locations = 0
    for c in range(1,largest_colocate_count+1):
        unique_locations += colocate_counts[c]//c
        print("{0:6d} are each occupied by {1:2d} waypoints.".format(colocate_counts[c]//c, c), flush=True)
    print("Unique locations: " + str(unique_locations), flush=True)

if args.errorcheck:
    print("!!! DATA CHECK SUCCESSFUL !!!", flush=True)

print("Finish: " + str(datetime.datetime.now()))
print("Total run time: " + et.et())
