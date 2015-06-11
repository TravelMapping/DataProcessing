#!/usr/bin/env python3
# Travel Mapping Project, Jim Teresco, 2015
"""Python code to read .csv and .wpt files and prepare for
adding to the Travel Mapping Project database.

(c) 2015, Jim Teresco

This module defines classes to represent the contents of a
.csv file that lists the highways within a system, and a
.wpt file that lists the waypoints for a given highway.
"""

import datetime
import re

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

    def sql_insert_command(self,tablename,root,id):
        """return sql command to insert into a table"""
        return "INSERT INTO " + tablename + " VALUES ('" + str(id) + "','" + self.label + "','" + str(self.lat) + "','" + str(self.lng) + "','" + root + "');";

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

    Root: the name of the .wpt file that lists the waypoints of the
    route, without the .wpt extension.

    AltRouteNames: (optional) comma-separated list former or other
    alternate route names that might appear in user list files.
    """
    def __init__(self,line):
        """initialize object from a .csv file line, but do not
        yet read in waypoint file"""
        self.line = line
        fields = line.split(";")
        if len(fields) != 8:
            print("Could not parse csv line: " + line)
        self.system = fields[0]
        self.region = fields[1]
        self.route = fields[2]
        self.banner = fields[3]
        self.abbrev = fields[4]
        self.city = fields[5].replace("'","''")
        self.root = fields[6]
        self.alt_route_names = fields[7].split(",")
        self.point_list = []
        self.labels_in_use = set()

    def __str__(self):
        """printable version of the object"""
        return self.line + " with " + str(len(self.point_list)) + " points"

    #def read_wpt(self,path="/Users/terescoj/travelmapping/work/old_chm_data"):
    def read_wpt(self,path="/Users/terescoj/travelmapping/HighwayData/chm_final"):
        """read data into the Route's waypoint list from a .wpt file"""
        print("read_wpt on " + str(self))
        self.point_list = []
        with open(path+"/"+self.system+"/"+self.root+".wpt", "rt") as file:
            lines = file.readlines()
        for line in lines:
            if len(line.rstrip('\n')) > 0:
                self.point_list.append(Waypoint(line.rstrip('\n')))

    def print_route(self):
        for point in self.point_list:
            print(str(point))

    def sql_insert_command(self,tablename):
        """return sql command to insert into a table"""
        # note: alt_route_names does not need to be in the db since
        # list preprocessing uses alt or canonical and no longer cares
        return "INSERT INTO " + tablename + " VALUES ('" + self.system + "','" + self.region + "','" + self.route + "','" + self.banner + "','" + self.abbrev + "','" + self.city + "','" + self.root + "');";

class HighwaySystem:
    """This class encapsulates the contents of one .csv file
    that represents the collection of highways within a system.

    See Route for information about the fields of a .csv file

    Each HighwaySystem is also designated as active or inactive via
    the parameter active, defaulting to true
    """
    #def __init__(self,systemname,path="/Users/terescoj/travelmapping/work/old_chm_data",active=True):
    def __init__(self,systemname,country,fullname,color,active,path="/Users/terescoj/travelmapping/HighwayData/chm_final/_systems"):
        self.route_list = []
        self.systemname = systemname
        self.country = country
        self.fullname = fullname
        self.color = color
        self.active = active
        with open(path+"/"+systemname+".csv","rt") as file:
            lines = file.readlines()
        # ignore the first line of field names
        lines.pop(0)
        for line in lines:
            print(systemname)
            self.route_list.append(Route(line.rstrip('\n')))
        file.close()

class TravelerList:
    """This class encapsulates the contents of one .list file
    that represents the travels of one individual user.

    A list file consists of lines of 4 values:
    region route_name start_waypoint end_waypoint

    which indicates that the user has traveled the highway names
    route_name in the given region between the waypoints named
    start_waypoint end_waypoint
    """

    def __init__(self,travelername,systems,path="/Users/terescoj/travelmapping/work/list_files"):
        self.list_entries = []
        self.traveler_name = travelername
        with open(path+"/"+travelername+".list","rt") as file:
            lines = file.readlines()
        file.close()

        print("Processing " + travelername)

        self.log_entries = []

        for line in lines:
            line = line.strip()
            if len(line) == 0:
                continue
            fields = re.split(' +',line)
            if len(fields) != 4:
                self.log_entries.append("Incorrect format line: " + line)
                continue

            # find the root that matches in some system and when we do, match labels
            lineDone = False
            for h in systems:
                for r in h.route_list:
                    if r.region.lower() != fields[0].lower():
                        continue
                    route_entry = fields[1].lower()
                    route_match = False
                    for a in r.alt_route_names:
                        if route_entry == a.lower():
                            self.log_entries.append("Note: replacing deprecated route name " + fields[1] + " with canonical name " + r.route + r.banner + r.abbrev + " in line " + line)
                            route_match = True
                            break
                    if route_match or (r.route + r.banner + r.abbrev).lower() == route_entry:
                        lineDone = True  # we'll either have success or failure here
                        if not h.active:
                            self.log_entries.append("Ignoring line matching highway in inactive system: " + line)
                            break
                        print("Route match with " + str(r))
                        # r is a route match, r.root is our root, and we need to find
                        # canonical labels, ignoring case and leading "+" or "*" when matching
                        canonical_labels = []
                        for w in r.point_list:
                            lower_label = w.label.lower().strip("+*")
                            if fields[2].lower() == lower_label or fields[3].lower() == lower_label:
                                canonical_labels.append(w.label)
                                r.labels_in_use.add(lower_label.upper())
                            else:
                                for alt in w.alt_labels:
                                    lower_label = alt.lower().strip("+")
                                    if fields[2].lower() == lower_label or fields[3].lower() == lower_label:
                                        canonical_labels.append(w.label)
                                        r.labels_in_use.add(lower_label.upper())
                        if len(canonical_labels) != 2:
                            self.log_entries.append("Waypoint label(s) not found in line: " + line)
                        else:
                            self.list_entries.append(ClinchedSegment(line, r.root, canonical_labels[0], canonical_labels[1]))
                    
                if lineDone:
                    break
            if not lineDone:
                self.log_entries.append("Unknown region/highway combo in line: " + line)
        self.log_entries.append("Processed " + str(len(self.list_entries)) + " good lines.")
       
    def write_log(self,path="."):
        logfile = open(path+"/"+self.traveler_name+".log","wt")
        logfile.write("Log file created at: " + str(datetime.datetime.now()) + "\n")
        for line in self.log_entries:
            logfile.write(line + "\n")
        logfile.close()

class ClinchedSegment:
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


#route_example = Route("usai;NY;I-684;;Pur;Purchase, NY;ny.i684pur;I-684_S")
#print(route_example)
#hs_example = HighwaySystem("usade")
#print(*hs_example.route_list)

# Execution code starts here
#
# First, give lists of active and development systems to be included.
# Might want these to be read from a config file in the future.
#active_systems = [ 'cannb', 'cannsc', 'cannsf', 'cannst', 'cannt', 'canon', 'canonf', 'canpe',
#                   'canqca', 'cantch', 'canyt', 'usaaz', 'usact', 'usadc', 'usade', 'usahi',
#                   'usai', 'usaia', 'usaib', 'usaid', 'usaif', 'usail', 'usaks', 'usaky3',
#                   'usaky', 'usama', 'usamd', 'usame', 'usami', 'usamn', 'usamo', 'usanc',
#                   'usand', 'usane', 'usanh', 'usanj', 'usansf', 'usanv', 'usany', 'usaoh',
#                   'usaok', 'usaor', 'usapa', 'usari', 'usasd', 'usasf', 'usaus', 'usausb', 'usawa',
#                   'usawi', 'usawv' ]
#devel_systems = [ 'usaak', 'usamt', 'usanm', 'usaut', 'usavt' ]
# Also list of travelers in the system
traveler_ids = [ 'terescoj', 'Bickendan', 'drfrankenstein', 'imgoph', 'master_son',
                 'mojavenc', 'oscar_voss', 'rickmastfan67', 'sammi', 'si404',
                 'sipes23' ]
#traveler_ids = [ 'little' ]

# Create a list of HighwaySystem objects, one per system in systems.csv file
highway_systems = []
with open("/Users/terescoj/travelmapping/HighwayData/systems.csv", "rt") as file:
    lines = file.readlines()

lines.pop(0)  # ignore header line for now
for line in lines:
    fields = line.rstrip('\n').split(";")
    if len(fields) != 5:
        print("Could not parse csv line: " + line)
    highway_systems.append(HighwaySystem(fields[0], fields[1], fields[2].replace("'","''"), fields[3], fields[4] != 'yes'))

#for h in active_systems:
#    highway_systems.append(HighwaySystem(h))
#for h in devel_systems:
#    highway_systems.append(HighwaySystem(h,active=False))

# Next, read all of the .wpt files for each HighwaySystem
for h in highway_systems:
    for r in h.route_list:
        r.read_wpt()
        #print(str(r))
        #r.print_route()

# Create a list of TravelerList objects, one per person
traveler_lists = []

for t in traveler_ids:
    traveler_lists.append(TravelerList(t,highway_systems))

# write log files for traveler lists
for t in traveler_lists:
    t.write_log()

# Once all data is read in and processed, create a .sql file that will 
# create all of the DB tables to be used by other parts of the project
sqlfile = open('siteupdate.sql','w')
sqlfile.write('USE TravelMapping\n')

# we have to drop tables in the right order to avoid foreign key errors
sqlfile.write('DROP TABLE IF EXISTS wpAltNames;\n')
sqlfile.write('DROP TABLE IF EXISTS waypoints;\n')
sqlfile.write('DROP TABLE IF EXISTS routes;\n')
sqlfile.write('DROP TABLE IF EXISTS systems;\n')

# first, a table of the systems, consisting of the system name in the
# field 'name', the system's country code, its full name, the default
# color for its mapping, and a boolean indicating if the system is
# active for mapping in the project in the field 'active'
sqlfile.write('CREATE TABLE systems (systemName VARCHAR(10), countryCode CHAR(3), fullName VARCHAR(50), color VARCHAR(10), active BOOLEAN, PRIMARY KEY(systemName));\n')
for h in highway_systems:
    active = 0;
    if h.active:
        active = 1;
    sqlfile.write("INSERT INTO systems VALUES ('" + h.systemname + "','" +  h.country + "','" +  h.fullname + "','" +  h.color + "','" + str(active) + "');\n")

# next, a table of highways, with the same fields as in the first line
sqlfile.write('CREATE TABLE routes (systemName VARCHAR(10), region VARCHAR(3), route VARCHAR(16), banner VARCHAR(3), abbrev VARCHAR(3), city VARCHAR(32), root VARCHAR(32), PRIMARY KEY(root), FOREIGN KEY (systemName) REFERENCES systems(systemName));\n')
for h in highway_systems:
    for r in h.route_list:
        sqlfile.write(r.sql_insert_command('routes') + "\n")
        # this info could go into the DB, but maybe it's better to put it right
        # into some other file to be made available to developers
        if len(r.labels_in_use) > 0:
            print("Labels in use for " + str(r) + ":\n" + str(r.labels_in_use))

# Now, a table with raw highway route data, also table for alternate names

sqlfile.write('CREATE TABLE waypoints (pointId INTEGER, pointName VARCHAR(20), latitude DOUBLE, longitude DOUBLE, root VARCHAR(32), PRIMARY KEY(pointId), FOREIGN KEY (root) REFERENCES routes(root));\n')
sqlfile.write('CREATE TABLE wpAltNames (pointId INTEGER, altPointName VARCHAR(20), FOREIGN KEY (pointId) REFERENCES waypoints(pointId));\n')
point_num = 0
for h in highway_systems:
    for r in h.route_list:
        for w in r.point_list:
            sqlfile.write(w.sql_insert_command('waypoints', r.root, point_num) + "\n")
            for a in w.alt_labels:
                sqlfile.write("INSERT INTO wpAltNames VALUES ('" + str(point_num) + "', '" + a + "');\n");
            point_num+=1

sqlfile.close()
