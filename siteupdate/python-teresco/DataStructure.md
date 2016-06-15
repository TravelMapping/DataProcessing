# Data Strucutures built in siteupdate.py

### General information data
`continents`, `countries`, and `regions` are all simple lists of arrays, containing the fields from the corresponding csv files.

### Highway Data structures

The global `highway_systems` is a list of `HighwaySystem` objects, one per system in `systems.csv`

Each `HighwaySystem` object has 
* the csv fields from its entry in `systems.csv' (system name, country, full name, color, tier, level)
* a list of `Route` objects in its field `route_list`
* a list of `ConnectedRoute` objects in its `con_route_list` field
* a `dict` in its `mileage_by_region` field whose keys are region codes and values are number of miles of highway in this system in that region.

Each `Route` object has
* the csv fields from its entry in the `hwy_data/_systems/XXXX.csv` file where this route is listed (region, route, banner, abbreviation, city, wpt file root, and a list of alternate names)
* a list of `Waypoint` objects in its field `point_list`, one per waypoint in the route
* a `set` in its field `labels_in_use`, which lists each label in this route used in any user's list file
* a list of `HighwaySegment` objects in its field `segment_list` connecting pairs of consecutive waypoints in this route
* a field `mileage` that gives the total mileage of the route

Each `ConnectedRoute` object has
* the csv fields from its entry in the `hwy_data/_systems/XXXX_con.csv` file where this route is listed (route, banner, group name)
* a list of wpt file roots in its field `roots`, one per `Route` that makes up this `ConnectedRoute`
* a field `mileage` that gives the total mileage of the connected route

