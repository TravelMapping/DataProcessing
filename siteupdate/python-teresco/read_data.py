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

    def sql_insert_command(self,tablename,root):
        """return sql command to insert into a table"""
        return "INSERT INTO " + tablename + " VALUES ('" + self.label + "','" + str(self.lat) + "','" + str(self.lng) + "','" + root + "');";

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
        self.alt_route_names = fields[7] #.split(",")
        self.point_list = []

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
        return "INSERT INTO " + tablename + " VALUES ('" + self.system + "','" + self.region + "','" + self.route + "','" + self.banner + "','" + self.abbrev + "','" + self.city + "','" + self.root + "','" + self.alt_route_names + "');";

class HighwaySystem:
    """This class encapsulates the contents of one .csv file
    that represents the collection of highways within a system.

    See Route for information about the fields of a .csv file

    Each HighwaySystem is also designated as active or inactive via
    the parameter active, defaulting to true
    """
    #def __init__(self,systemname,path="/Users/terescoj/travelmapping/work/old_chm_data",active=True):
    def __init__(self,systemname,path="/Users/terescoj/travelmapping/HighwayData/chm_final/_systems",active=True):
        self.route_list = []
        self.systemname = systemname
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

        # not happy with this design, but...
        # open a log file for this user's error messages
        logfile = open(travelername+".log", "w")

        for line in lines:
            line = line.strip()
            print("Line: [" + line + "]")
            fields = line.split(' ')
            if len(fields) != 4:
                logfile.write("Incorrect format line: " + line + "\n")
                continue

            # find the root that matches in some system and when we do, match labels
            lineDone = False
            for h in systems:
                for r in h.route_list:
                    if r.region.lower() == fields[0].lower() and (r.route + r.banner + r.abbrev).lower() == fields[1].lower():
                        lineDone = True  # we'll either have success or failure here
                        if not h.active:
                            logfile.write("Line matches highway in inactive system: " + line + "\n")
                            break
                        print("Route match with " + str(r))
                        # r is a route match, r.root is our root, and we need to find
                        # canonical labels, ignoring case and leading "+" or "*" when matching
                        canonical_labels = []
                        for w in r.point_list:
                            print("Considering waypoint: " + str(w))
                            lower_label = w.label.lower().strip("+*")
                            print("lower_label: " + lower_label + " compared with " + fields[2] + " and " + fields[3])
                            if fields[2].lower() == lower_label or fields[3].lower() == lower_label:
                                print("Match!  Appending " + w.label)
                                canonical_labels.append(w.label)
                            else:
                                print("Considering alt labels.")
                                for alt in w.alt_labels:
                                    lower_label = alt.lower().strip("+")
                                    if fields[2].lower() == lower_label or fields[3].lower() == lower_label:
                                        canonical_labels.append(w.label)
                        if len(canonical_labels) != 2:
                            logfile.write("Waypoint label(s) not found in line: " + line + "\n")
                        else:
                            self.list_entries.append(ClinchedSegment(line, r.root, canonical_labels[0], canonical_labels[1]))
                    
                if lineDone:
                    break
            if not lineDone:
                logfile.write("Unknown region/highway combo in line: " + line + "\n")
        logfile.write("Processed " + str(len(self.list_entries)) + " good lines.\n")
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
active_systems = [ 'cannb', 'cannsc', 'cannsf', 'cannst', 'cannt', 'canon', 'canonf', 'canpe',
                   'canqca', 'cantch', 'canyt', 'usaaz', 'usact', 'usadc', 'usade', 'usahi',
                   'usai', 'usaia', 'usaib', 'usaid', 'usaif', 'usail', 'usaks', 'usaky3',
                   'usaky', 'usama', 'usamd', 'usame', 'usami', 'usamn', 'usamo', 'usanc',
                   'usand', 'usane', 'usanh', 'usanj', 'usansf', 'usanv', 'usany', 'usaoh',
                   'usaok', 'usaor', 'usapa', 'usari', 'usasd', 'usasf', 'usaus', 'usawa',
                   'usawi', 'usawv' ] #, 'usausb' ]
devel_systems = [ 'usaak', 'usamt', 'usanm', 'usaut', 'usavt' ]
# Also list of travelers in the system
traveler_ids = [ 'terescoj', 'little' ]

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

# Create a list of TravelerList objects, one per person
traveler_lists = []

for t in traveler_ids:
    traveler_lists.append(TravelerList(t,highway_systems))

# Once all data is read in and processed, create a .sql file that will 
# create all of the DB tables to be used by other parts of the project
sqlfile = open('siteupdate.sql','w')
sqlfile.write('USE TravelMapping\n')

# we have to drop tables in the right order to avoid foreign key errors
sqlfile.write('DROP TABLE IF EXISTS waypoints;\n')
sqlfile.write('DROP TABLE IF EXISTS wpAltNames;\n')
sqlfile.write('DROP TABLE IF EXISTS routes;\n')
sqlfile.write('DROP TABLE IF EXISTS systems;\n')

# first, a table of the systems, consisting of just the system name in
# the field 'name', and a boolean indicating if the system is active for
# mapping in the project in the field 'active'
sqlfile.write('CREATE TABLE systems (systemName VARCHAR(10), active BOOLEAN, PRIMARY KEY(systemName));\n')
for h in highway_systems:
    active = 0;
    if h.active:
        active = 1;
    sqlfile.write("INSERT INTO systems VALUES ('" + h.systemname + "','" + str(active) + "');\n")

# next, a table of highways, with the same fields as in the first line
sqlfile.write('CREATE TABLE routes (systemName VARCHAR(10), region VARCHAR(3), route VARCHAR(16), banner VARCHAR(3), abbrev VARCHAR(3), city VARCHAR(32), root VARCHAR(16), altroutenames VARCHAR(32), PRIMARY KEY(root), FOREIGN KEY (systemName) REFERENCES systems(systemName));\n')
for h in highway_systems:
    for r in h.route_list:
        sqlfile.write(r.sql_insert_command('routes') + "\n")

# Now, a table with raw highway route data, also table for alternate names

sqlfile.write('CREATE TABLE waypoints (pointName VARCHAR(20), latitude DOUBLE, longitude DOUBLE, root VARCHAR(16), FOREIGN KEY (root) REFERENCES routes(root));\n')
sqlfile.write('CREATE TABLE wpAltNames (altPointName VARCHAR(20), root VARCHAR(16), pointName VARCHAR(20), FOREIGN KEY (root) REFERENCES routes(root), FOREIGN KEY (pointName) REFERENCES waypoints(pointName));\n')
for h in highway_systems:
    for r in h.route_list:
        for w in r.point_list:
            sqlfile.write(w.sql_insert_command('waypoints', r.root) + "\n")
            for a in w.alt_labels:
                sqlfile.write("INSERT INTO wpAltNames VALUES ('" + a + "', '" + r.root + "', '" + w.label + "');\n");

sqlfile.close()
