#!/usr/bin/env python3
# Travel Mapping Project, Jim Teresco, 2015
"""Python code to read .csv and .wpt files and prepare for
adding to the Travel Mapping Project database.

(c) 2015, Jim Teresco

This module defines classes to represent the contents of a
.csv file that lists the highways within a system, and a
.wpt file that lists the waypoints for a given highway.
"""

class Waypoint:
    """This class encapsulates the information about a single waypoint
    from a .wpt file.

    The line consists of one or more labels, at most one of which can
    be a "regular" label.  Others are "hidden" labels and must begin with
    a '+'.  Then an OSM URL which encodes the latitude and longitude.
    """
    def __init__(self,line):
        """initialize object from a .wpt file line"""
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
        lat_string = url_parts[1].split("&")[0] # chop off "&lon"
        lng_string = url_parts[2].split("&")[0] # chop off possible "&zoom"
        self.lat = float(lat_string)
        self.lng = float(lng_string)

    def __str__(self):
        ans = self.label
        if len(self.alt_labels) > 0:
            ans = ans + " [alt: " + str(self.alt_labels) + "]"
        ans = ans + " (" + str(self.lat) + "," + str(self.lng) + ")"
        return ans

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
    normally the same as the name of the .csv file.

    Region: the project region or subdivision in which the
    route belongs.

    Route: the route name as would be specified in user lists

    Banner: the (optional) banner on the route such as 'Alt',
    'Bus', or 'Trk'.

    Abbrev: (optional) for bannered routes or routes in multiple
    sections, the 3-letter abbrevation for the city or other place
    that is used to identify the segment.

    City: (optional) the full name to be displayed for the Abbrev
    above.

    Route: the name of the .wpt file that lists the waypoints of the
    route, without the .wpt extension.

    AltRouteNames: (optional) comma-separated list former or other
    altername route names that might appear in user list files.
    """
    def __init__(self,line):
        """initialize object from a .csv file line, but do not
        yet read in waypoint file"""
        self.line = line
        fields = line.split(";")
        self.system = fields[0]
        self.region = fields[1]
        self.route = fields[2]
        self.banner = fields[3]
        self.abbrev = fields[4]
        self.city = fields[5]
        self.route = fields[6]
        self.alt_route_names = fields[7].split(",")
        self.point_list = []

    def __str__(self):
        """printable version of the object"""
        return self.line + " with " + str(len(self.point_list)) + " points"

    def read_wpt(self,path="/Users/terescoj/travelmapping/work/old_chm_data"):
        """read data into the Route's waypoint list from a .wpt file"""
        self.point_list = []
        with open(path+"/"+self.system+"/"+self.route+".wpt", "rt") as file:
            lines = file.readlines()
        for line in lines:
            if len(line.rstrip('\n')) > 0:
                self.point_list.append(Waypoint(line.rstrip('\n')))

    def print_route(self):
        for point in self.point_list:
            print(str(point))

class HighwaySystem:
    """This class encapsulates the contents of one .csv file
    that represents the collection of highways within a system.

    See Route for information about the fields of a .csv file

    Each HighwaySystem is also designated as active or inactive via
    the parameter active, defaulting to true
    """
    def __init__(self,systemname,path="/Users/terescoj/travelmapping/work/old_chm_data",active=True):
        self.route_list = []
        self.active = active
        with open(path+"/"+systemname+".csv","rt") as file:
            lines = file.readlines()
        lines.pop(0)
        for line in lines:
            self.route_list.append(Route(line.rstrip('\n')))
        file.close()
        
#route_example = Route("usai;NY;I-684;;Pur;Purchase, NY;ny.i684pur;I-684_S")
#print(route_example)
#hs_example = HighwaySystem("usade")
#print(*hs_example.route_list)

# Execution code starts here
#
# First, give lists of active and development systems to be included.
# Might want these to be read from a config file in the future.
active_systems = [ 'usai', 'usaib', 'usaif', 'usaus', 'usausb' ]
devel_systems = [ 'usanm', 'usavt' ]

# Create a list of HighwaySystem objects, one per system above
highway_systems = []
for h in active_systems:
    highway_systems.append(HighwaySystem(h))
for h in devel_systems:
    highway_systems.append(HighwaySystem(h,active=False))

# Next, read all of the .wpt files for each HighwaySystem
for h in highway_systems:
    for r in h.route_list:
        r.read_wpt()
        #print(str(r))
        #r.print_route()

