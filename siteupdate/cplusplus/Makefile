all : siteupdate siteupdateST

siteupdate : \
  siteupdateMT.o \
  classes/Arguments/Arguments.o \
  classes/ClinchedDBValues/ClinchedDBValues.o \
  classes/ConnectedRoute/ConnectedRoute.o \
  classes/DatacheckEntry/DatacheckEntry.o \
  classes/DBFieldLength/DBFieldLength.o \
  classes/ElapsedTime/ElapsedTime.o \
  classes/ErrorList/ErrorList.o \
  classes/GraphGeneration/GraphListEntry.o \
  classes/GraphGeneration/HGEdge.o \
  classes/GraphGeneration/HGVertex.o \
  classes/GraphGeneration/HighwayGraphMT.o \
  classes/GraphGeneration/PlaceRadius.o \
  classes/HighwaySegment/HighwaySegment.o \
  classes/Region/Region.o \
  classes/Route/Route.o \
  classes/TravelerList/TravelerList.o \
  classes/Waypoint/Waypoint.o \
  classes/Waypoint/canonical_waypoint_name/canonical_waypoint_name.o \
  classes/WaypointQuadtree/WaypointQuadtree.o \
  functions/crawl_hwy_data.o \
  functions/double_quotes.o \
  functions/format_clinched_mi.o \
  functions/lower.o \
  functions/split.o \
  functions/sql_fileMT.o \
  functions/upper.o \
  functions/valid_num_str.o \
  threads/threads.o
	g++ -std=c++11 -pthread -D threading_enabled -o siteupdate siteupdateMT.o \
	  classes/Arguments/Arguments.o \
	  classes/ClinchedDBValues/ClinchedDBValues.o \
	  classes/ConnectedRoute/ConnectedRoute.o \
	  classes/DatacheckEntry/DatacheckEntry.o \
	  classes/DBFieldLength/DBFieldLength.o \
	  classes/ElapsedTime/ElapsedTime.o \
	  classes/ErrorList/ErrorList.o \
	  classes/GraphGeneration/GraphListEntry.o \
	  classes/GraphGeneration/HGEdge.o \
	  classes/GraphGeneration/HGVertex.o \
	  classes/GraphGeneration/HighwayGraphMT.o \
	  classes/GraphGeneration/PlaceRadius.o \
	  classes/HighwaySegment/HighwaySegment.o \
	  classes/Region/Region.o \
	  classes/Route/Route.o \
	  classes/TravelerList/TravelerList.o \
	  classes/Waypoint/Waypoint.o \
	  classes/Waypoint/canonical_waypoint_name/canonical_waypoint_name.o \
	  classes/WaypointQuadtree/WaypointQuadtree.o \
	  functions/crawl_hwy_data.o \
	  functions/double_quotes.o \
	  functions/format_clinched_mi.o \
	  functions/lower.o \
	  functions/split.o \
	  functions/sql_fileMT.o \
	  functions/upper.o \
	  functions/valid_num_str.o \
	  threads/threads.o

siteupdateST : \
  siteupdateST.o \
  classes/Arguments/Arguments.o \
  classes/ClinchedDBValues/ClinchedDBValues.o \
  classes/ConnectedRoute/ConnectedRoute.o \
  classes/DatacheckEntry/DatacheckEntry.o \
  classes/DBFieldLength/DBFieldLength.o \
  classes/ElapsedTime/ElapsedTime.o \
  classes/ErrorList/ErrorList.o \
  classes/GraphGeneration/GraphListEntry.o \
  classes/GraphGeneration/HGEdge.o \
  classes/GraphGeneration/HGVertex.o \
  classes/GraphGeneration/HighwayGraphST.o \
  classes/GraphGeneration/PlaceRadius.o \
  classes/HighwaySegment/HighwaySegment.o \
  classes/Region/Region.o \
  classes/Route/Route.o \
  classes/TravelerList/TravelerList.o \
  classes/Waypoint/Waypoint.o \
  classes/Waypoint/canonical_waypoint_name/canonical_waypoint_name.o \
  classes/WaypointQuadtree/WaypointQuadtree.o \
  functions/crawl_hwy_data.o \
  functions/double_quotes.o \
  functions/format_clinched_mi.o \
  functions/lower.o \
  functions/split.o \
  functions/sql_fileST.o \
  functions/upper.o \
  functions/valid_num_str.o
	g++ -std=c++11 -o siteupdateST siteupdateST.o \
	  classes/Arguments/Arguments.o \
	  classes/ClinchedDBValues/ClinchedDBValues.o \
	  classes/ConnectedRoute/ConnectedRoute.o \
	  classes/DatacheckEntry/DatacheckEntry.o \
	  classes/DBFieldLength/DBFieldLength.o \
	  classes/ElapsedTime/ElapsedTime.o \
	  classes/ErrorList/ErrorList.o \
	  classes/GraphGeneration/GraphListEntry.o \
	  classes/GraphGeneration/HGEdge.o \
	  classes/GraphGeneration/HGVertex.o \
	  classes/GraphGeneration/HighwayGraphST.o \
	  classes/GraphGeneration/PlaceRadius.o \
	  classes/HighwaySegment/HighwaySegment.o \
	  classes/Region/Region.o \
	  classes/Route/Route.o \
	  classes/TravelerList/TravelerList.o \
	  classes/Waypoint/Waypoint.o \
	  classes/Waypoint/canonical_waypoint_name/canonical_waypoint_name.o \
	  classes/WaypointQuadtree/WaypointQuadtree.o \
	  functions/crawl_hwy_data.o \
	  functions/double_quotes.o \
	  functions/format_clinched_mi.o \
	  functions/lower.o \
	  functions/split.o \
	  functions/sql_fileST.o \
	  functions/upper.o \
	  functions/valid_num_str.o

siteupdateMT.o : siteupdate.cpp tasks/*.cpp tasks/subgraphs/*.cpp
	g++ -std=c++11 -pthread -o siteupdateMT.o -D threading_enabled -c siteupdate.cpp

siteupdateST.o : siteupdate.cpp tasks/*.cpp tasks/subgraphs/*.cpp
	g++ -std=c++11 -pthread -o siteupdateST.o -c siteupdate.cpp

classes/Arguments/Arguments.o : classes/Arguments/Arguments.h classes/Arguments/Arguments.cpp
	g++ -std=c++11 -o classes/Arguments/Arguments.o -c classes/Arguments/Arguments.cpp

classes/ClinchedDBValues/ClinchedDBValues.o : classes/ClinchedDBValues/ClinchedDBValues.h classes/ClinchedDBValues/ClinchedDBValues.cpp
	g++ -std=c++11 -o classes/ClinchedDBValues/ClinchedDBValues.o -c classes/ClinchedDBValues/ClinchedDBValues.cpp

classes/ConnectedRoute/ConnectedRoute.o : classes/ConnectedRoute/ConnectedRoute.h classes/ConnectedRoute/ConnectedRoute.cpp \
  classes/DBFieldLength/DBFieldLength.h \
  classes/ErrorList/ErrorList.h \
  classes/HighwaySystem/HighwaySystem.h \
  classes/Route/Route.h \
  functions/double_quotes.h \
  functions/lower.h \
  functions/split.h
	g++ -std=c++11 -o classes/ConnectedRoute/ConnectedRoute.o -c classes/ConnectedRoute/ConnectedRoute.cpp

classes/DatacheckEntry/DatacheckEntry.o : classes/DatacheckEntry/DatacheckEntry.h classes/DatacheckEntry/DatacheckEntry.cpp \
  classes/Route/Route.h
	g++ -std=c++11 -o classes/DatacheckEntry/DatacheckEntry.o -c classes/DatacheckEntry/DatacheckEntry.cpp

classes/DBFieldLength/DBFieldLength.o : classes/DBFieldLength/DBFieldLength.h classes/DBFieldLength/DBFieldLength.cpp
	g++ -std=c++11 -o classes/DBFieldLength/DBFieldLength.o -c classes/DBFieldLength/DBFieldLength.cpp

classes/ElapsedTime/ElapsedTime.o : classes/ElapsedTime/ElapsedTime.h classes/ElapsedTime/ElapsedTime.cpp
	g++ -std=c++11 -o classes/ElapsedTime/ElapsedTime.o -c classes/ElapsedTime/ElapsedTime.cpp

classes/ErrorList/ErrorList.o : classes/ErrorList/ErrorList.h classes/ErrorList/ErrorList.cpp
	g++ -std=c++11 -o classes/ErrorList/ErrorList.o -c classes/ErrorList/ErrorList.cpp

classes/GraphGeneration/GraphListEntry.o : classes/GraphGeneration/GraphListEntry.h classes/GraphGeneration/GraphListEntry.cpp \
  classes/GraphGeneration/PlaceRadius.h \
  classes/HighwaySystem/HighwaySystem.h \
  classes/Region/Region.h
	g++ -std=c++11 -o classes/GraphGeneration/GraphListEntry.o -c classes/GraphGeneration/GraphListEntry.cpp

classes/GraphGeneration/HGEdge.o : classes/GraphGeneration/HGEdge.h classes/GraphGeneration/HGEdge.cpp \
  classes/GraphGeneration/HGVertex.h \
  classes/GraphGeneration/HighwayGraph.h \
  classes/HighwaySegment/HighwaySegment.h \
  classes/HighwaySystem/HighwaySystem.h \
  classes/Route/Route.h \
  classes/Waypoint/Waypoint.h \
  templates/contains.cpp
	g++ -std=c++11 -o classes/GraphGeneration/HGEdge.o -c classes/GraphGeneration/HGEdge.cpp

classes/GraphGeneration/HGVertex.o : classes/GraphGeneration/HGVertex.h classes/GraphGeneration/HGVertex.cpp \
  classes/DatacheckEntry/DatacheckEntry.h \
  classes/GraphGeneration/HGEdge.h \
  classes/HighwaySystem/HighwaySystem.h \
  classes/Region/Region.h \
  classes/Route/Route.h \
  classes/Waypoint/Waypoint.h
	g++ -std=c++11 -o classes/GraphGeneration/HGVertex.o -c classes/GraphGeneration/HGVertex.cpp

classes/GraphGeneration/HighwayGraphMT.o : classes/GraphGeneration/*.h classes/GraphGeneration/HighwayGraph.cpp \
  classes/DatacheckEntry/DatacheckEntry.h \
  classes/ElapsedTime/ElapsedTime.h \
  classes/HighwaySegment/HighwaySegment.h \
  classes/HighwaySystem/HighwaySystem.h \
  classes/Region/Region.h \
  classes/Route/Route.h \
  classes/TravelerList/TravelerList.h \
  classes/Waypoint/Waypoint.h \
  templates/contains.cpp \
  templates/set_intersection.cpp
	g++ -std=c++11 -o classes/GraphGeneration/HighwayGraphMT.o -D threading_enabled -c classes/GraphGeneration/HighwayGraph.cpp

classes/GraphGeneration/HighwayGraphST.o : classes/GraphGeneration/*.h classes/GraphGeneration/HighwayGraph.cpp \
  classes/DatacheckEntry/DatacheckEntry.h \
  classes/ElapsedTime/ElapsedTime.h \
  classes/HighwaySegment/HighwaySegment.h \
  classes/HighwaySystem/HighwaySystem.h \
  classes/Region/Region.h \
  classes/Route/Route.h \
  classes/TravelerList/TravelerList.h \
  classes/Waypoint/Waypoint.h \
  templates/contains.cpp \
  templates/set_intersection.cpp
	g++ -std=c++11 -o classes/GraphGeneration/HighwayGraphST.o -c classes/GraphGeneration/HighwayGraph.cpp

classes/GraphGeneration/PlaceRadius.o : classes/GraphGeneration/PlaceRadius.h classes/GraphGeneration/PlaceRadius.cpp \
  classes/GraphGeneration/HGEdge.h \
  classes/GraphGeneration/HGVertex.h \
  classes/GraphGeneration/HighwayGraph.h \
  classes/Waypoint/Waypoint.h
	g++ -std=c++11 -o classes/GraphGeneration/PlaceRadius.o -c classes/GraphGeneration/PlaceRadius.cpp

classes/HighwaySegment/HighwaySegment.o : classes/HighwaySegment/HighwaySegment.h classes/HighwaySegment/HighwaySegment.cpp \
  classes/HighwaySegment/compute_stats_t.cpp \
  classes/HighwaySystem/HighwaySystem.h \
  classes/Route/Route.h \
  classes/TravelerList/TravelerList.h \
  classes/Waypoint/Waypoint.h
	g++ -std=c++11 -o classes/HighwaySegment/HighwaySegment.o -c classes/HighwaySegment/HighwaySegment.cpp

classes/Region/Region.o : classes/Region/Region.h classes/Region/Region.cpp \
  classes/DBFieldLength/DBFieldLength.h \
  classes/ErrorList/ErrorList.h \
  functions/split.h
	g++ -std=c++11 -o classes/Region/Region.o -c classes/Region/Region.cpp

classes/Route/Route.o : classes/Route/Route.h classes/Route/Route.cpp \
  classes/Route/compute_stats_r.cpp \
  classes/Route/read_wpt.cpp \
  classes/DBFieldLength/DBFieldLength.h \
  classes/ErrorList/ErrorList.h \
  classes/HighwaySegment/HighwaySegment.h \
  classes/HighwaySystem/HighwaySystem.h \
  classes/Region/Region.h \
  classes/TravelerList/TravelerList.h \
  classes/Waypoint/Waypoint.h \
  functions/double_quotes.h \
  functions/lower.h \
  functions/split.h \
  functions/upper.h
	g++ -std=c++11 -o classes/Route/Route.o -c classes/Route/Route.cpp

classes/TravelerList/TravelerList.o : classes/TravelerList/TravelerList.h classes/TravelerList/TravelerList.cpp \
  classes/TravelerList/mark_chopped_route_segments.cpp \
  classes/TravelerList/mark_connected_route_segments.cpp \
  classes/TravelerList/splitregion.cpp \
  classes/TravelerList/userlog.cpp \
  classes/Arguments/Arguments.h \
  classes/ConnectedRoute/ConnectedRoute.h \
  classes/DBFieldLength/DBFieldLength.h \
  classes/ErrorList/ErrorList.h \
  classes/HighwaySegment/HighwaySegment.h \
  classes/HighwaySystem/HighwaySystem.h \
  classes/Region/Region.h \
  classes/Route/Route.h \
  classes/Waypoint/Waypoint.h \
  functions/upper.h
	g++ -std=c++11 -o classes/TravelerList/TravelerList.o -c classes/TravelerList/TravelerList.cpp

classes/Waypoint/Waypoint.o : classes/Waypoint/Waypoint.h classes/Waypoint/Waypoint.cpp \
  classes/DatacheckEntry/DatacheckEntry.h \
  classes/DBFieldLength/DBFieldLength.h \
  classes/HighwaySystem/HighwaySystem.h \
  classes/Route/Route.h \
  functions/valid_num_str.h
	g++ -std=c++11 -o classes/Waypoint/Waypoint.o -c classes/Waypoint/Waypoint.cpp

classes/Waypoint/canonical_waypoint_name/canonical_waypoint_name.o : classes/Waypoint/canonical_waypoint_name/*.cpp \
  classes/Waypoint/Waypoint.h \
  classes/Route/Route.h \
  templates/contains.cpp
	g++ -std=c++11 -o classes/Waypoint/canonical_waypoint_name/canonical_waypoint_name.o -c classes/Waypoint/canonical_waypoint_name/canonical_waypoint_name.cpp

classes/WaypointQuadtree/WaypointQuadtree.o : classes/WaypointQuadtree/WaypointQuadtree.h classes/WaypointQuadtree/WaypointQuadtree.cpp \
  classes/ErrorList/ErrorList.h \
  classes/HighwaySystem/HighwaySystem.h \
  classes/Route/Route.h \
  classes/Waypoint/Waypoint.h \
  templates/contains.cpp
	g++ -std=c++11 -o classes/WaypointQuadtree/WaypointQuadtree.o -c classes/WaypointQuadtree/WaypointQuadtree.cpp

functions/crawl_hwy_data.o : functions/crawl_hwy_data.h functions/crawl_hwy_data.cpp
	g++ -std=c++11 -o functions/crawl_hwy_data.o -c functions/crawl_hwy_data.cpp

functions/double_quotes.o : functions/double_quotes.h functions/double_quotes.cpp
	g++ -std=c++11 -o functions/double_quotes.o -c functions/double_quotes.cpp

functions/format_clinched_mi.o : functions/format_clinched_mi.h functions/format_clinched_mi.cpp
	g++ -std=c++11 -o functions/format_clinched_mi.o -c functions/format_clinched_mi.cpp

functions/lower.o : functions/lower.h functions/lower.cpp
	g++ -std=c++11 -o functions/lower.o -c functions/lower.cpp

functions/split.o : functions/split.h functions/split.cpp
	g++ -std=c++11 -o functions/split.o -c functions/split.cpp

functions/sql_fileMT.o : functions/sql_file.h functions/sql_file.cpp \
  functions/double_quotes.h \
  classes/Arguments/Arguments.h \
  classes/ClinchedDBValues/ClinchedDBValues.h \
  classes/ConnectedRoute/ConnectedRoute.h \
  classes/DatacheckEntry/DatacheckEntry.h \
  classes/DBFieldLength/DBFieldLength.h \
  classes/ElapsedTime/ElapsedTime.h \
  classes/GraphGeneration/GraphListEntry.h \
  classes/HighwaySegment/HighwaySegment.h \
  classes/HighwaySystem/HighwaySystem.h \
  classes/Region/Region.h \
  classes/Route/Route.h \
  classes/TravelerList/TravelerList.h \
  classes/Waypoint/Waypoint.h
	g++ -std=c++11 -o functions/sql_fileMT.o -D threading_enabled -c functions/sql_file.cpp

functions/sql_fileST.o : functions/sql_file.h functions/sql_file.cpp \
  functions/double_quotes.h \
  classes/Arguments/Arguments.h \
  classes/ClinchedDBValues/ClinchedDBValues.h \
  classes/ConnectedRoute/ConnectedRoute.h \
  classes/DatacheckEntry/DatacheckEntry.h \
  classes/DBFieldLength/DBFieldLength.h \
  classes/ElapsedTime/ElapsedTime.h \
  classes/GraphGeneration/GraphListEntry.h \
  classes/HighwaySegment/HighwaySegment.h \
  classes/HighwaySystem/HighwaySystem.h \
  classes/Region/Region.h \
  classes/Route/Route.h \
  classes/TravelerList/TravelerList.h \
  classes/Waypoint/Waypoint.h
	g++ -std=c++11 -o functions/sql_fileST.o -c functions/sql_file.cpp

functions/upper.o : functions/upper.h functions/upper.cpp
	g++ -std=c++11 -o functions/upper.o -c functions/upper.cpp

functions/valid_num_str.o : functions/valid_num_str.h functions/valid_num_str.cpp
	g++ -std=c++11 -o functions/valid_num_str.o -c functions/valid_num_str.cpp

threads/threads.o : threads/threads.h threads/*.cpp
	g++ -std=c++11 -o threads/threads.o -c threads/threads.cpp
