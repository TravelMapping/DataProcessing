#include "threads.h"
#include "../classes/GraphGeneration/GraphListEntry.h"
#include "../classes/GraphGeneration/HighwayGraph.h"
#include "../classes/HighwaySegment/HighwaySegment.h"
#include "../classes/HighwaySystem/HighwaySystem.h"
#include "../classes/Region/Region.h"
#include "../classes/Route/Route.h"
#include "../classes/Args/Args.h"
#include "../classes/TravelerList/TravelerList.h"
#include "../classes/Waypoint/Waypoint.h"
#include "../classes/WaypointQuadtree/WaypointQuadtree.h"
#include <iostream>

#include "CompStatsThread.cpp"
#include "ConcAugThread.cpp"
#include "MasterTmgThread.cpp"
#include "NmpMergedThread.cpp"
#include "NmpSearchThread.cpp"
#include "ReadListThread.cpp"
#include "ReadWptThread.cpp"
#include "RteIntThread.cpp"
#include "StatsCsvThread.cpp"
#include "SubgraphThread.cpp"
#include "UserLogThread.cpp"
