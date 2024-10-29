OS := $(shell uname)
ifeq ($(OS), Linux)
	OS := $(shell hostnamectl | grep 'Operating System:' | sed -e 's~.*: ~~' -e 's~[ \.]~_~g' -e 's~[^A-Za-z0-9_]~~g')
endif

CXX = clang++
CXXFLAGS = -O3 -std=c++11 -isystem /usr/local/include -isystem /opt/local/include -Wno-comment -Wno-dangling-else -Wno-logical-op-parentheses

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
  classes/ConnectedRoute/datacheck.o \
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
  classes/Region/read_csvs.o \
  classes/Route/Route.o \
  classes/TravelerList/TravelerList.o \
  classes/TravelerList/userlog.o \
  classes/Waypoint/Waypoint.o \
  classes/Waypoint/canonical_waypoint_name/canonical_waypoint_name.o \
  functions/crawl_rte_data.o \
  functions/rdstats.o \
  functions/route_and_label_logs.o \
  functions/tmstring.o

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
	@$(CXX) $(CXXFLAGS) -D $(OS) -o $@ -D threading_enabled -c $<
%ST.o:
	@echo $@
	@$(CXX) $(CXXFLAGS) -D $(OS) -o $@ -c $<
%.o:
	@echo $@
	@$(CXX) $(CXXFLAGS) -D $(OS) -o $@ -c $<

siteupdate: $(MTObjects) $(CommonObjects)
	@echo $@
	@$(CXX) $(CXXFLAGS) -D $(OS) -pthread -o siteupdate $(MTObjects) $(CommonObjects)
siteupdateST: $(STObjects) $(CommonObjects)
	@echo $@
	@$(CXX) $(CXXFLAGS) -D $(OS) -o siteupdateST $(STObjects) $(CommonObjects)

clean:
	@rm -f siteupdate siteupdateST `find . -name \*.d` `find . -name \*.o`
