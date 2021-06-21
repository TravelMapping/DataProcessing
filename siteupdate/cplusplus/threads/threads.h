class ClinchedDBValues;
class ElapsedTime;
class ErrorList;
class HighwayGraph;
class WaypointQuadtree;
#include <list>
#include <mutex>

void CompStatsRThread(unsigned int, std::mutex*);
void CompStatsTThread(unsigned int, std::mutex*);
void ConcAugThread   (unsigned int, std::mutex*, std::list<std::string>*);
void MasterTmgThread(HighwayGraph*, std::mutex*, std::mutex*, WaypointQuadtree*, ElapsedTime*);
void NmpMergedThread (unsigned int, std::mutex*);
void NmpSearchThread (unsigned int, std::mutex*, WaypointQuadtree*);
void ReadListThread  (unsigned int, std::mutex*, ErrorList*);
void ReadWptThread   (unsigned int, std::mutex*, ErrorList*, WaypointQuadtree*);
void RteIntThread    (unsigned int, std::mutex*, ErrorList*);
void StatsCsvThread  (unsigned int, std::mutex*);
void SubgraphThread  (unsigned int, std::mutex*, std::mutex*, HighwayGraph*, WaypointQuadtree*, ElapsedTime*);
void UserLogThread   (unsigned int, std::mutex*, ClinchedDBValues*, const double, const double);
