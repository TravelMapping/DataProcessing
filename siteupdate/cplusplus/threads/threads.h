class ElapsedTime;
class ErrorList;
class HGVertex;
class HighwayGraph;
class WaypointQuadtree;
#include <mutex>
#include <vector>

void CompStatsThread (unsigned int, std::mutex*);
void ConcAugThread   (unsigned int, std::mutex*, std::vector<std::string>*);
void MasterTmgThread(HighwayGraph*, std::mutex*, std::mutex*, WaypointQuadtree*, ElapsedTime*);
void NmpMergedThread (unsigned int, std::mutex*);
void NmpSearchThread (unsigned int, std::mutex*, WaypointQuadtree*);
void ReadListThread  (unsigned int, std::mutex*, ErrorList*);
void ReadWptThread   (unsigned int, std::mutex*, ErrorList*, WaypointQuadtree*);
void RteIntThread    (unsigned int, std::mutex*, ErrorList*);
void StatsCsvThread  (unsigned int, std::mutex*);
void SubgraphThread  (unsigned int, std::mutex*, std::mutex*, HighwayGraph*, WaypointQuadtree*, ElapsedTime*);
void UserLogThread   (unsigned int, std::mutex*, const double, const double);
void VtxFmtThread    (unsigned int, std::vector<HGVertex>*);
