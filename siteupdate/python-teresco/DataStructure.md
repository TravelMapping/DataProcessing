# Data Strucutures built in siteupdate.py

### General information data
`continents`, `countries`, and `regions` are all simple lists of arrays, containing the fields from the corresponding csv files.

### Highway Data structures

`highway_systems` is a list of `HighwaySystem` objects, one per system in `systems.csv`

Each `HighwaySystem` object has 
* the csv fields from its entry in `systems.csv'
* a list of `Route` objects in its field `route_list`
* a list of `ConnectedRoute` objects in its `con_route_list` field
* a `dict` in its `mileage_by_region` field whose keys are region codes and values are number of miles of highway in this system in that region.
