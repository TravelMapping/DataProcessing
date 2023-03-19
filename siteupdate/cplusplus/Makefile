CXX = clang++
CXXFLAGS = -O3 -std=c++11 -Wno-comment -Wno-dangling-else -Wno-logical-op-parentheses

STObjects = \
  classes/GraphGeneration/HighwayGraphST.o \
  classes/Route/read_wptST.o \
  classes/Route/store_traveled_segmentsST.o \
  classes/WaypointQuadtree/WaypointQuadtreeST.o \
  functions/allbyregionactiveonlyST.o \
  functions/allbyregionactivepreviewST.o \
  functions/sql_fileST.o \
  siteupdateST.o

MTObjects = $(STObjects:ST.o=MT.o) \
  threads/threads.o

CommonObjects = \
  classes/Args/Args.o \
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
  classes/Region/compute_stats.o \
  classes/Route/Route.o \
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
	@rm -f siteupdate siteupdateST `find . -name \*.d` `find . -name \*.o`
