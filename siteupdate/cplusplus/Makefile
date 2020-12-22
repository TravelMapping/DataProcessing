MTObjects = siteupdateMT.o classes/GraphGeneration/HighwayGraphMT.o functions/sql_fileMT.o threads/threads.o
STObjects = siteupdateST.o classes/GraphGeneration/HighwayGraphST.o functions/sql_fileST.o
CommonObjects = \
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
  classes/GraphGeneration/PlaceRadius.o \
  classes/HighwaySegment/HighwaySegment.o \
  classes/HighwaySystem/HighwaySystem.o \
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
  functions/upper.o \
  functions/valid_num_str.o

.PHONY: all clean
all: siteupdate siteupdateST

%MT.d: %.cpp ; g++ -std=c++11 -MM -D threading_enabled $< | sed -r "s~.*:~$*MT.o:~" > $@
%ST.d: %.cpp ; g++ -std=c++11 -MM $< | sed -r "s~.*:~$*ST.o:~" > $@
%.d  : %.cpp ; g++ -std=c++11 -MM $< | sed -r "s~.*:~$*.o:~" > $@

include $(MTObjects:.o=.d)
include $(STObjects:.o=.d)
include $(CommonObjects:.o=.d)

%MT.o: ; g++ -std=c++11 -o $@ -D threading_enabled -c $<
%ST.o: ; g++ -std=c++11 -o $@ -c $<
%.o  : ; g++ -std=c++11 -o $@ -c $<

siteupdate: $(MTObjects) $(CommonObjects)
	@echo Linking siteupdate...
	@g++ -std=c++11 -pthread -o siteupdate $(MTObjects) $(CommonObjects)
siteupdateST: $(STObjects) $(CommonObjects)
	@echo Linking siteupdateST...
	@g++ -std=c++11 -o siteupdateST $(STObjects) $(CommonObjects)

clean:
	rm -f $(MTObjects)       $(STObjects)       $(CommonObjects)
	rm -f $(MTObjects:.o=.d) $(STObjects:.o=.d) $(CommonObjects:.o=.d)
