CXX = clang++
CXXFLAGS = -std=c++11 -Wno-comment -Wno-dangling-else -Wno-logical-op-parentheses

MTObjects = siteupdateMT.o functions/sql_fileMT.o threads/threads.o \
  classes/GraphGeneration/HighwayGraphMT.o \
  classes/WaypointQuadtree/WaypointQuadtreeMT.o \
  functions/allbyregionactiveonlyMT.o \
  functions/allbyregionactivepreviewMT.o

STObjects = siteupdateST.o functions/sql_fileST.o \
  classes/GraphGeneration/HighwayGraphST.o \
  classes/WaypointQuadtree/WaypointQuadtreeST.o \
  functions/allbyregionactiveonlyST.o \
  functions/allbyregionactivepreviewST.o

CommonObjects = \
  classes/Args/Args.o \
  classes/ClinchedDBValues/ClinchedDBValues.o \
  classes/ConnectedRoute/ConnectedRoute.o \
  classes/Datacheck/Datacheck.o \
  classes/DBFieldLength/DBFieldLength.o \
  classes/ElapsedTime/ElapsedTime.o \
  classes/ErrorList/ErrorList.o \
  classes/GraphGeneration/GraphListEntry.o \
  classes/GraphGeneration/HGEdge.o \
  classes/GraphGeneration/HGVertex.o \
  classes/GraphGeneration/PlaceRadius.o \
  classes/HighwaySegment/HighwaySegment.o \
  classes/HighwaySystem/HighwaySystem.o \
  classes/HighwaySystem/route_integrity.o \
  classes/Region/Region.o \
  classes/Route/Route.o \
  classes/Route/read_wpt.o \
  classes/TravelerList/TravelerList.o \
  classes/TravelerList/userlog.o \
  classes/Waypoint/Waypoint.o \
  classes/Waypoint/canonical_waypoint_name/canonical_waypoint_name.o \
  functions/crawl_hwy_data.o \
  functions/double_quotes.o \
  functions/format_clinched_mi.o \
  functions/lower.o \
  functions/split.o \
  functions/strd.o \
  functions/upper.o \
  functions/valid_num_str.o

.PHONY: all clean
all: siteupdate siteupdateST

%MT.d: %.cpp
	@echo $@
	@$(CXX) $(CXXFLAGS) -MM -D threading_enabled $< | sed -r "s~.*:~$*MT.o:~" > $@
%ST.d: %.cpp
	@echo $@
	@$(CXX) $(CXXFLAGS) -MM $< | sed -r "s~.*:~$*ST.o:~" > $@
%.d  : %.cpp
	@echo $@
	@$(CXX) $(CXXFLAGS) -MM $< | sed -r "s~.*:~$*.o:~" > $@

-include $(MTObjects:.o=.d)
-include $(STObjects:.o=.d)
-include $(CommonObjects:.o=.d)

%MT.o:
	@echo $@
	@$(CXX) $(CXXFLAGS) -o $@ -D threading_enabled -c $<
%ST.o:
	@echo $@
	@$(CXX) $(CXXFLAGS) -o $@ -c $<
%.o:
	@echo $@
	@$(CXX) $(CXXFLAGS) -o $@ -c $<

siteupdate: $(MTObjects) $(CommonObjects)
	@echo $@
	@$(CXX) $(CXXFLAGS) -pthread -o siteupdate $(MTObjects) $(CommonObjects)
siteupdateST: $(STObjects) $(CommonObjects)
	@echo $@
	@$(CXX) $(CXXFLAGS) -o siteupdateST $(STObjects) $(CommonObjects)

clean:
	@rm -f $(MTObjects)       $(STObjects)       $(CommonObjects)
	@rm -f $(MTObjects:.o=.d) $(STObjects:.o=.d) $(CommonObjects:.o=.d)
