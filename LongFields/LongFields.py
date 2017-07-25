#!/usr/bin/env python3
# Travel Mapping Project, Jim Teresco, 2015, 2016, 2017
"""Python code to read .csv files to find the longest values
at each field in each kind of csv.

(c) 2017, Jim Teresco, Travel Mapping
"""

path = "../../HighwayData"

# first, check top-level csvs
files = [ "continents.csv", "countries.csv", "datacheckfps.csv",
          "regions.csv", "systems.csv", "systemupdates.csv",
          "updates.csv" ]
for filename in files:
    print("Checking " + filename)
    file = open(path + "/" + filename, "rt", encoding="utf-8")
    lines = file.readlines()
    file.close()
    headers = lines[0].rstrip('\n').split(";")
    longest = lines[1].rstrip('\n').split(";")
    for linenum in range(2,len(lines)):
        if lines[linenum].startswith("#"):
            continue
        line = lines[linenum].rstrip('\n').split(";")
        for pos in range(len(line)):
            if len(line[pos]) > len(longest[pos]):
                longest[pos] = line[pos]
    for pos in range(len(headers)):
        print(headers[pos] + ": <" + longest[pos] + "> (" + str(len(longest[pos])) + ")")

# check _systems csv files
sysfile = open(path + "/systems.csv", "rt", encoding="utf-8")
lines = sysfile.readlines()
sysfile.close()
systems = []
for linenum in range(1,len(lines)):
    if lines[linenum].startswith("#"):
        continue
    systems.append(lines[linenum].split(";")[0])

# first, regular csvs
firstFile = True
for s in systems:
    csvfile = open(path + "/hwy_data/_systems/" + s + ".csv", "rt", encoding="utf-8")
    lines = csvfile.readlines()
    csvfile.close()
    if firstFile:
        headers = lines[0].rstrip('\n').split(";")
        longest = lines[1].rstrip('\n').split(";")
        firstFile = False
    for linenum in range(1,len(lines)):
        line = lines[linenum].rstrip('\n').split(";")
        for pos in range(len(line)):
            if len(line[pos]) > len(longest[pos]):
                longest[pos] = line[pos]
print("For system '.csv' files:")
for pos in range(len(headers)):
    print(headers[pos] + ": <" + longest[pos] + "> (" + str(len(longest[pos])) + ")")

# finally, connected route csvs
firstFile = True
for s in systems:
    csvfile = open(path + "/hwy_data/_systems/" + s + "_con.csv", "rt", encoding="utf-8")
    lines = csvfile.readlines()
    csvfile.close()
    if firstFile:
        headers = lines[0].rstrip('\n').split(";")
        longest = lines[1].rstrip('\n').split(";")
        firstFile = False
    for linenum in range(1,len(lines)):
        line = lines[linenum].rstrip('\n').split(";")
        for pos in range(len(line)):
            if len(line[pos]) > len(longest[pos]):
                longest[pos] = line[pos]
print("For system '_con.csv' files:")
for pos in range(len(headers)):
    print(headers[pos] + ": <" + longest[pos] + "> (" + str(len(longest[pos])) + ")")
