# Data Strucutures built in siteupdate.py

### General information data
`continents`, `countries`, and `regions` are all simple lists of arrays, containing the fields from the corresponding csv files.

### Highway Data structures

The global `highway_systems` is a list of `HighwaySystem` objects, one per system in `systems.csv`.

Each `HighwaySystem` object has 
* the csv fields from its entry in `systems.csv' (system name, country, full name, color, tier, level)
* a list of `Route` objects in its field `route_list`
* a list of `ConnectedRoute` objects in its `con_route_list` field
* a `dict` in its `mileage_by_region` field whose keys are region codes and values are number of miles of highway in this system in that region.

Each `Route` object has
* the csv fields from its entry in the `hwy_data/_systems/XXXX.csv` file where this route is listed (region, route, banner, abbreviation, city, wpt file root, and a list of alternate names)
* a list of `Waypoint` objects in its field `point_list`, one per waypoint in the route (one per line in the wpt file)
* a `set` in its field `labels_in_use`, which lists each label in this route used in any user's list file
* a list of `HighwaySegment` objects in its field `segment_list` connecting pairs of consecutive waypoints in this route
* a field `mileage` that gives the total mileage of the route.

Each `ConnectedRoute` object has
* the csv fields from its entry in the `hwy_data/_systems/XXXX_con.csv` file where this route is listed (route, banner, group name)
* a list of wpt file roots in its field `roots`, one per `Route` that makes up this `ConnectedRoute`
* a field `mileage` that gives the total mileage of the connected route.

Each `Waypoint` object has
* a field `route` that refers to the `Route` object representing the route of which this waypoint is a part
* a field `label` which is a string containing the primary label of this waypoint
* a boolean field `is_hidden` indicating if this is a hidden label (i.e., that its primary label starts with a '+')
* a list of strings in `alt_labels` containing any alternate (usually former) labels for this waypoint that might still be used in some user list files
* two fields `lat` and `lng` containing the latitude and longitude of the waypoint
* a list `colocated` that contains references to other `Waypoint` objects that occupy the same point.  If `None` this means there are no other waypoints at the same location.  Any set of `Waypoint` objects that are co-located at a given point will share a reference to the same list.

Each `HighwaySegment` object has
* two fields `w1` and `w2`, that refer to the two `Waypoint` objects at the segment's endpoints
* a field `route` that refers to the `Route` object representing the route of which this highway segment is a part
* a list `concurrent` that contains references to other `HighwaySegment` objects that are concurrent.  If `None` this means there are no other highway segment at the same location.  Any set of `HighwaySegment` objects that are concurrent will share a reference to the same list.
* a list `clinched_by` that contains references to `TravelerList` objects of any traveler whose list file marks this segment as traveled.

Each `TravelerList` object has
* a list `list_entries`, each entry of which contains a reference to a `ClinchedSegmentEntry` object that represents one line of the traveler's list file
* a set `clinched_segments` that contains references to all of the `HighwaySegment` objects that have been clinched by this traveler
* a string `traveler_name`, which is the name of the list file, minus the .list extension
* a list of strings `log_entries`, where entries are placed that will be written to the user's log file during a site update
* three `dict` objects where mileage stats are stored:
  * `active_preview_mileage_by_region` has keys which are region codes, and values that are the traverler's overall clinchecd mileage on all active or preview systems in that region
  * `active_only_mileage_by_region` has keys which are region codes, and values that are the traverler's overall clinchecd mileage on only active systems in that region
  * `system_region_mileages` has keys which are system codes, and values that are themselves `dict` objects that have keys which are region codes and values that are the traveler's clinched mileage within that region for the system.
  * 

