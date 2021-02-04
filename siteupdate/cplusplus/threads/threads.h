class Arguments;
class ClinchedDBValues;
class ElapsedTime;
class ErrorList;
class GraphListEntry;
class HighwayGraph;
class HighwaySystem;
class TravelerList;
class WaypointQuadtree;
#include <list>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

void CompStatsRThread(unsigned int, std::list<HighwaySystem*>*, std::list<HighwaySystem*>::iterator*, std::mutex*);
void CompStatsTThread(unsigned int, std::list<TravelerList *>*, std::list<TravelerList *>::iterator*, std::mutex*);
void ConcAugThread   (unsigned int, std::list<TravelerList *>*, std::list<TravelerList *>::iterator*, std::mutex*, std::list<std::string>*);
void LabelConThread  (unsigned int, std::list<HighwaySystem*>*, std::list<HighwaySystem*>::iterator*, std::mutex*, ErrorList*);
void MasterTmgThread(HighwayGraph*, std::vector<GraphListEntry>*, std::string, std::list<TravelerList*>*, size_t*, std::mutex*, std::mutex*, WaypointQuadtree*, ElapsedTime*);
void NmpMergedThread (unsigned int, std::list<HighwaySystem*>*, std::list<HighwaySystem*>::iterator*, std::mutex*, std::string*);
void NmpSearchThread (unsigned int, std::list<HighwaySystem*>*, std::list<HighwaySystem*>::iterator*, std::mutex*, WaypointQuadtree*);
void ReadListThread  (unsigned int, std::list<std::string>*,    std::unordered_map<std::string, std::string**>*, std::list<std::string>::iterator*, std::list<TravelerList*>*, std::mutex*, Arguments*, ErrorList*);
void ReadWptThread   (unsigned int, std::list<HighwaySystem*>*, std::list<HighwaySystem*>::iterator*, std::mutex*, std::string, ErrorList*, std::unordered_set<std::string>*, WaypointQuadtree*);
void SubgraphThread  (unsigned int, HighwayGraph*, std::vector<GraphListEntry>*, size_t*, std::mutex*, std::mutex*, std::string, WaypointQuadtree*, ElapsedTime*);
void UserLogThread   (unsigned int, std::list<TravelerList *>*, std::list<TravelerList*>::iterator*, std::mutex*, ClinchedDBValues*, const double, const double, std::list<HighwaySystem*>*, std::string);
