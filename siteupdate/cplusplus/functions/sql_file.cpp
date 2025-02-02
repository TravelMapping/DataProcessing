#define FMT_HEADER_ONLY
#include "sql_file.h"
#include "tmstring.h"
#include "../classes/Args/Args.h"
#include "../classes/ConnectedRoute/ConnectedRoute.h"
#include "../classes/Datacheck/Datacheck.h"
#include "../classes/DBFieldLength/DBFieldLength.h"
#include "../classes/ElapsedTime/ElapsedTime.h"
#include "../classes/GraphGeneration/GraphListEntry.h"
#include "../classes/HighwaySegment/HighwaySegment.h"
#include "../classes/HighwaySystem/HighwaySystem.h"
#include "../classes/Region/Region.h"
#include "../classes/Route/Route.h"
#include "../classes/TravelerList/TravelerList.h"
#include "../classes/Waypoint/Waypoint.h"
#include <fmt/format.h>
#include <fstream>

void sqlfile1
    (	ElapsedTime *et,
	std::list<std::string*> *updates,
	std::list<std::string*> *systemupdates,
	std::mutex* term_mtx
    ){	char fstr[65];
	// Once all data is read in and processed, create a .sql file that will
	// create all of the DB tables to be used by other parts of the project
	std::ofstream sqlfile(Args::databasename+".sql");
	// Note: removed "USE" line, DB name must be specified on the mysql command line

	// we have to drop tables in the right order to avoid foreign key errors
	sqlfile << "DROP TABLE IF EXISTS datacheckErrors;\n";
	sqlfile << "DROP TABLE IF EXISTS clinchedConnectedRoutes;\n";
	sqlfile << "DROP TABLE IF EXISTS clinchedRoutes;\n";
	sqlfile << "DROP TABLE IF EXISTS clinchedOverallMileageByRegion;\n";
	sqlfile << "DROP TABLE IF EXISTS clinchedSystemMileageByRegion;\n";
	sqlfile << "DROP TABLE IF EXISTS listEntries;\n";
	sqlfile << "DROP TABLE IF EXISTS overallMileageByRegion;\n";
	sqlfile << "DROP TABLE IF EXISTS systemMileageByRegion;\n";
	sqlfile << "DROP TABLE IF EXISTS clinched;\n";
	sqlfile << "DROP TABLE IF EXISTS segments;\n";
	sqlfile << "DROP TABLE IF EXISTS waypoints;\n";
	sqlfile << "DROP TABLE IF EXISTS connectedRouteRoots;\n";
	sqlfile << "DROP TABLE IF EXISTS connectedRoutes;\n";
	sqlfile << "DROP TABLE IF EXISTS routes;\n";
	sqlfile << "DROP TABLE IF EXISTS systems;\n";
	sqlfile << "DROP TABLE IF EXISTS updates;\n";
	sqlfile << "DROP TABLE IF EXISTS systemUpdates;\n";
	sqlfile << "DROP TABLE IF EXISTS regions;\n";
	sqlfile << "DROP TABLE IF EXISTS countries;\n";
	sqlfile << "DROP TABLE IF EXISTS continents;\n";

	// first, continents, countries, and regions
      #ifndef threading_enabled
	std::cout << et->et() << "...continents, countries, regions" << std::endl;
      #endif
	sqlfile << "CREATE TABLE continents (code VARCHAR(" << DBFieldLength::continentCode
		<< "), name VARCHAR(" << DBFieldLength::continentName
		<< "), PRIMARY KEY(code));\n";
	sqlfile << "INSERT INTO continents VALUES\n";
	bool first = 1;
	for (size_t c = 0; c < Region::continents.size()-1; c++)
	{	if (!first) sqlfile << ',';
		first = 0;
		sqlfile << "('" << Region::continents[c].first << "','" << Region::continents[c].second << "')\n";
	}
	sqlfile << ";\n";

	sqlfile << "CREATE TABLE countries (code VARCHAR(" << DBFieldLength::countryCode
		<< "), name VARCHAR(" << DBFieldLength::countryName
		<< "), PRIMARY KEY(code));\n";
	sqlfile << "INSERT INTO countries VALUES\n";
	first = 1;
	for (size_t c = 0; c < Region::countries.size()-1; c++)
	{	if (!first) sqlfile << ',';
		first = 0;
		sqlfile << "('" << Region::countries[c].first << "','" << double_quotes(Region::countries[c].second) << "')\n";
	}
	sqlfile << ";\n";

	sqlfile << "CREATE TABLE regions (code VARCHAR(" << DBFieldLength::regionCode
		<< "), name VARCHAR(" << DBFieldLength::regionName
		<< "), country VARCHAR(" << DBFieldLength::countryCode
		<< "), continent VARCHAR(" << DBFieldLength::continentCode
		<< "), regiontype VARCHAR(" << DBFieldLength::regiontype
		<< "), ";
	sqlfile << "PRIMARY KEY(code), FOREIGN KEY (country) REFERENCES countries(code), FOREIGN KEY (continent) REFERENCES continents(code));\n";
	sqlfile << "INSERT INTO regions VALUES\n";
	first = 1;
	for (Region *r = Region::allregions.data, *dummy = Region::allregions.end()-1; r < dummy; r++)
	{	if (!first) sqlfile << ',';
		first = 0;
		sqlfile << "('" << r->code << "','" << double_quotes(r->name)
			<< "','" << r->country_code() << "','" << r->continent_code()
			<< "','" << r->type << "')\n";
	}
	sqlfile << ";\n";

	// next, a table of the systems, consisting of the system name in the
	// field 'name', the system's country code, its full name, the default
	// color for its mapping, a level (one of active, preview, devel), and
	// a boolean indicating if the system is active for mapping in the
	// project in the field 'active'
      #ifndef threading_enabled
	std::cout << et->et() << "...systems" << std::endl;
      #endif
	sqlfile << "CREATE TABLE systems (systemName VARCHAR(" << DBFieldLength::systemName
		<< "), countryCode CHAR(" << DBFieldLength::countryCode
		<< "), fullName VARCHAR(" << DBFieldLength::systemFullName
		<< "), color VARCHAR(" << DBFieldLength::color
		<< "), level VARCHAR(" << DBFieldLength::level
		<< "), tier INTEGER, csvOrder INTEGER, PRIMARY KEY(systemName));\n";
	sqlfile << "INSERT INTO systems VALUES\n";
	first = 1;
	unsigned int csvOrder = 0;
	for (HighwaySystem& h : HighwaySystem::syslist)
	{	if (!first) sqlfile << ',';
		first = 0;
		sqlfile << "('" << h.systemname << "','" << h.country->first << "','"
			<< double_quotes(h.fullname) << "','" << h.color << "','" << h.level_name()
			<< "','" << h.tier << "','" << csvOrder << "')\n";
		csvOrder += 1;
	}
	sqlfile << ";\n";

	// next, a table of highways, with the same fields as in the first line
      #ifndef threading_enabled
	std::cout << et->et() << "...routes" << std::endl;
      #endif
	sqlfile << "CREATE TABLE routes (systemName VARCHAR(" << DBFieldLength::systemName
		<< "), region VARCHAR(" << DBFieldLength::regionCode
		<< "), route VARCHAR(" << DBFieldLength::route
		<< "), banner VARCHAR(" << DBFieldLength::banner
		<< "), abbrev VARCHAR(" << DBFieldLength::abbrev
		<< "), city VARCHAR(" << DBFieldLength::city
		<< "), root VARCHAR(" << DBFieldLength::root
		<< "), mileage FLOAT, rootOrder INTEGER, csvOrder INTEGER, PRIMARY KEY(root), FOREIGN KEY (systemName) REFERENCES systems(systemName));\n";
	sqlfile << "INSERT INTO routes VALUES\n";
	first = 1;
	csvOrder = 0;
	for (HighwaySystem& h : HighwaySystem::syslist)
	  for (Route& r : h.routes)
	  {	if (!first) sqlfile << ',';
		first = 0;
		*fmt::format_to(fstr, "{}", r.mileage) = 0;
		sqlfile << "('" << r.system->systemname << "','" << r.region->code << "','" << r.route << "','" << r.banner << "','" << r.abbrev
			<< "','" << double_quotes(r.city) << "','" << r.root << "','" << fstr << "','" << r.rootOrder << "','" << csvOrder << "')\n";
		csvOrder += 1;
	  }
	sqlfile << ";\n";

	// connected routes table, but only first "root" in each in this table
      #ifndef threading_enabled
	std::cout << et->et() << "...connectedRoutes" << std::endl;
      #endif
	sqlfile << "CREATE TABLE connectedRoutes (systemName VARCHAR(" << DBFieldLength::systemName
		<< "), route VARCHAR(" << DBFieldLength::route
		<< "), banner VARCHAR(" << DBFieldLength::banner
		<< "), groupName VARCHAR(" << DBFieldLength::city
		<< "), firstRoot VARCHAR(" << DBFieldLength::root
		<< "), mileage FLOAT, csvOrder INTEGER, PRIMARY KEY(firstRoot), FOREIGN KEY (firstRoot) REFERENCES routes(root));\n";
	sqlfile << "INSERT INTO connectedRoutes VALUES\n";
	first = 1;
	csvOrder = 0;
	for (HighwaySystem& h : HighwaySystem::syslist)
	  for (ConnectedRoute& cr : h.con_routes)
	  {	if (!first) sqlfile << ',';
		first = 0;
		*fmt::format_to(fstr, "','{}'", cr.mileage) = 0;
		sqlfile << "('" << cr.system->systemname << "','" << cr.route << "','" << cr.banner << "','" << double_quotes(cr.groupname)
			<< "','" << (cr.roots.size() ? cr.roots[0]->root.data() : "ERROR_NO_ROOTS") << fstr << ",'" << csvOrder << "')\n";
		csvOrder += 1;
	  }
	sqlfile << ";\n";

	// This table has remaining roots for any connected route
	// that connects multiple routes/roots
      #ifndef threading_enabled
	std::cout << et->et() << "...connectedRouteRoots" << std::endl;
      #endif
	sqlfile << "CREATE TABLE connectedRouteRoots (firstRoot VARCHAR(" << DBFieldLength::root
		<< "), root VARCHAR(" << DBFieldLength::root
		<< "), FOREIGN KEY (firstRoot) REFERENCES connectedRoutes(firstRoot));\n";
	first = 1;
	for (HighwaySystem& h : HighwaySystem::syslist)
	  for (ConnectedRoute& cr : h.con_routes)
	    for (unsigned int i = 1; i < cr.roots.size(); i++)
	    {	if (first) sqlfile << "INSERT INTO connectedRouteRoots VALUES\n";
		else sqlfile << ',';
		first = 0;
		sqlfile << "('" << cr.roots[0]->root << "','" << cr.roots[i]->root << "')\n";
	    }
	sqlfile << ";\n";

	// Now, a table with raw highway route data: list of points, in order, that define the route
      #ifndef threading_enabled
	std::cout << et->et() << "...waypoints" << std::endl;
      #endif
	sqlfile << "CREATE TABLE waypoints (pointId INTEGER, pointName VARCHAR(" << DBFieldLength::label
		<< "), latitude DOUBLE, longitude DOUBLE, root VARCHAR(" << DBFieldLength::root
		<< "), PRIMARY KEY(pointId), FOREIGN KEY (root) REFERENCES routes(root));\n";
	unsigned int point_num = 0;
	for (HighwaySystem& h : HighwaySystem::syslist)
	  for (Route& r : h.routes)
	  {	sqlfile << "INSERT INTO waypoints VALUES\n";
		first = 1;
		for (Waypoint& w : r.points)
		{	if (!first) sqlfile << ',';
			first = 0;
			sqlfile << "('" << point_num << "','" << w.label;
			*fmt::format_to(fstr,"','{:.15}",w.lat) = 0; sqlfile << fstr; if (w.lat==int(w.lat)) sqlfile << ".0";
			*fmt::format_to(fstr,"','{:.15}",w.lng) = 0; sqlfile << fstr; if (w.lng==int(w.lng)) sqlfile << ".0";
			sqlfile << "','" << r.root + "')\n";
			point_num+=1;
		}
		sqlfile << ";\n";
	  }

	// Build indices to speed latitude/longitude joins for intersecting highway queries
	sqlfile << "CREATE INDEX `latitude` ON waypoints(`latitude`);\n";
	sqlfile << "CREATE INDEX `longitude` ON waypoints(`longitude`);\n";

	// Table of all HighwaySegments.
      #ifndef threading_enabled
	std::cout << et->et() << "...segments" << std::endl;
      #endif
	sqlfile << "CREATE TABLE segments (segmentId INTEGER, waypoint1 INTEGER, waypoint2 INTEGER, root VARCHAR(" << DBFieldLength::root
		<< "), PRIMARY KEY (segmentId), FOREIGN KEY (waypoint1) REFERENCES waypoints(pointId), "
		<< "FOREIGN KEY (waypoint2) REFERENCES waypoints(pointId), FOREIGN KEY (root) REFERENCES routes(root));\n";
	unsigned int segment_num = point_num = 0;
	std::vector<std::pair<unsigned int, const char*>> clinched_list;
	for (HighwaySystem& h : HighwaySystem::syslist)
	  for (Route& r : h.routes)
	  {	sqlfile << "INSERT INTO segments VALUES\n";
		first = 1;
		for (HighwaySegment& s : r.segments)
		{	if (!first) sqlfile << ',';
			first = 0;
			sqlfile << "('" << segment_num;
			sqlfile	<< "','" << point_num++;
			sqlfile	<< "','" << point_num;
			sqlfile	<< "','" << r.root << "')\n";
			for (TravelerList *t : s.clinched_by)
			  clinched_list.emplace_back(segment_num, t->traveler_name.data());
			segment_num += 1;
		}
		sqlfile << ";\n";
		point_num++;
	  }

	// maybe a separate traveler table will make sense but for now, I'll just use
	// the name from the .list name
      #ifndef threading_enabled
	std::cout << et->et() << "...clinched" << std::endl;
      #endif
	sqlfile << "CREATE TABLE clinched (segmentId INTEGER, traveler VARCHAR(" << DBFieldLength::traveler
		<< "), FOREIGN KEY (segmentId) REFERENCES segments(segmentId));\n";
	for (size_t start = 0; start < clinched_list.size(); start += 10000)
	{	sqlfile << "INSERT INTO clinched VALUES\n";
		first = 1;
		for (size_t c = start; c < start+10000 && c < clinched_list.size(); c++)
		{	if (!first) sqlfile << ',';
			first = 0;
			sqlfile << "('" << clinched_list[c].first << "','" << clinched_list[c].second << "')\n";
		}
		sqlfile << ";\n";
	}

	// overall mileage by region data (with concurrencies accounted for,
	// active systems only then active+preview)
      #ifndef threading_enabled
	std::cout << et->et() << "...overallMileageByRegion" << std::endl;
      #endif
	sqlfile << "CREATE TABLE overallMileageByRegion (region VARCHAR(" << DBFieldLength::regionCode
		<< "), activeMileage DOUBLE, activePreviewMileage DOUBLE);\n";
	sqlfile << "INSERT INTO overallMileageByRegion VALUES\n";
	first = 1;
	for (Region& region : Region::allregions)
	{	if (region.active_only_mileage+region.active_preview_mileage == 0) continue;
		if (!first) sqlfile << ',';
		first = 0;
		*fmt::format_to(fstr, "','{}','{}')\n", region.active_only_mileage, region.active_preview_mileage) = 0;
		sqlfile << "('" << region.code << fstr;
	}
	sqlfile << ";\n";

	// system mileage by region data (with concurrencies accounted for,
	// active systems and preview systems only)
      #ifndef threading_enabled
	std::cout << et->et() << "...systemMileageByRegion" << std::endl;
      #endif
	sqlfile << "CREATE TABLE systemMileageByRegion (systemName VARCHAR(" << DBFieldLength::systemName
		<< "), region VARCHAR(" << DBFieldLength::regionCode
		<< "), mileage DOUBLE, FOREIGN KEY (systemName) REFERENCES systems(systemName));\n";
	sqlfile << "INSERT INTO systemMileageByRegion VALUES\n";
	first = 1;
	for (HighwaySystem& h : HighwaySystem::syslist)
	  if (h.active_or_preview())
	    for (std::pair<Region* const,double>& rm : h.mileage_by_region)
	    {	if (!first) sqlfile << ',';
		first = 0;
		*fmt::format_to(fstr, "','{}')\n", rm.second) = 0;
		sqlfile << "('" << h.systemname << "','" << rm.first->code << fstr;
	    }
	sqlfile << ";\n";

	// clinched overall mileage by region data (with concurrencies
	// accounted for, active systems and preview systems only)
      #ifndef threading_enabled
	std::cout << et->et() << "...clinchedOverallMileageByRegion" << std::endl;
      #endif
	sqlfile << "CREATE TABLE clinchedOverallMileageByRegion (region VARCHAR(" << DBFieldLength::regionCode
		<< "), traveler VARCHAR(" << DBFieldLength::traveler
		<< "), activeMileage DOUBLE, activePreviewMileage DOUBLE);\n";
	sqlfile << "INSERT INTO clinchedOverallMileageByRegion VALUES\n";
	first = 1;
	for (TravelerList& t : TravelerList::allusers)
	  for (std::pair<Region* const,double>& rm : t.active_preview_mileage_by_region)
	  {	if (!first) sqlfile << ',';
		first = 0;
		auto it = t.active_only_mileage_by_region.find(rm.first);
		double active_miles = (it != t.active_only_mileage_by_region.end()) ? it->second : 0;
		*fmt::format_to(fstr, "','{}','{}')\n", active_miles, rm.second) = 0;
		sqlfile << "('" << rm.first->code << "','" << t.traveler_name << fstr;
	  }
	sqlfile << ";\n";

	// clinched system mileage by region data (with concurrencies accounted
	// for, active systems and preview systems only)
      #ifndef threading_enabled
	std::cout << et->et() << "...clinchedSystemMileageByRegion" << std::endl;
      #endif
	sqlfile << "CREATE TABLE clinchedSystemMileageByRegion (systemName VARCHAR(" << DBFieldLength::systemName
		<< "), region VARCHAR(" << DBFieldLength::regionCode
		<< "), traveler VARCHAR(" << DBFieldLength::traveler
		<< "), mileage DOUBLE, FOREIGN KEY (systemName) REFERENCES systems(systemName));\n";
	sqlfile << "INSERT INTO clinchedSystemMileageByRegion VALUES\n";
	first = 1;
	for (TravelerList& t : TravelerList::allusers)
	  for (auto& csmbr : t.system_region_mileages)
	  {	auto& systemname = csmbr.first->systemname;
		for (auto& rm : csmbr.second)
		{	if (!first) sqlfile << ',';
			first = 0;
			*fmt::format_to(fstr, "{}", rm.second) = 0;
			sqlfile << "('" << systemname << "','" << rm.first->code << "','" << t.traveler_name << "','" << fstr << "')\n";
		}
	  }
	sqlfile << ";\n";

	// clinched mileage by connected route, active systems and preview
	// systems only
      #ifndef threading_enabled
	std::cout << et->et() << "...clinchedConnectedRoutes" << std::endl;
      #endif
	sqlfile << "CREATE TABLE clinchedConnectedRoutes (route VARCHAR(" << DBFieldLength::root
		<< "), traveler VARCHAR(" << DBFieldLength::traveler
		<< "), mileage FLOAT, clinched BOOLEAN, FOREIGN KEY (route) REFERENCES connectedRoutes(firstRoot));\n";
	for (TravelerList& t : TravelerList::allusers)
	{	if (t.ccr_values.empty()) continue;
		sqlfile << "INSERT INTO clinchedConnectedRoutes VALUES\n";
		first = 1;
		for (auto& rm : t.ccr_values)
		{	if (!first) sqlfile << ',';
			first = 0;
			*fmt::format_to(fstr, "{}", rm.second) = 0;
			sqlfile << "('" << rm.first->roots[0]->root << "','" << t.traveler_name << "','"
				<< fstr << "','" << (rm.second == rm.first->mileage) << "')\n";
		}
		sqlfile << ";\n";
	}

	// clinched mileage by route, active systems and preview systems only
      #ifndef threading_enabled
	std::cout << et->et() << "...clinchedRoutes" << std::endl;
      #endif
	sqlfile << "CREATE TABLE clinchedRoutes (route VARCHAR(" << DBFieldLength::root
		<< "), traveler VARCHAR(" << DBFieldLength::traveler
		<< "), mileage FLOAT, clinched BOOLEAN, FOREIGN KEY (route) REFERENCES routes(root));\n";
	for (TravelerList& t : TravelerList::allusers)
	{	if (t.cr_values.empty()) continue;
		sqlfile << "INSERT INTO clinchedRoutes VALUES\n";
		first = 1;
		for (std::pair<Route*,double>& rm : t.cr_values)
		{	if (!first) sqlfile << ',';
			first = 0;
			*fmt::format_to(fstr, "{}", rm.second) = 0;
			sqlfile << "('" << rm.first->root << "','" << t.traveler_name << "','"
				<< fstr << "','" << (rm.second >= rm.first->mileage) << "')\n";
		}
		sqlfile << ";\n";
	}

	// list entries
	  #ifndef threading_enabled
	std::cout << et->et() << "...listEntries" << std::endl;
	  #endif
	sqlfile << "CREATE TABLE listEntries (traveler VARCHAR(" << DBFieldLength::traveler
		<< "), description VARCHAR(" << DBFieldLength::listDescription
		<< "), includeInRanks BOOLEAN);\n";
	sqlfile << "INSERT INTO listEntries VALUES\n";
	first = 1;
	for (TravelerList& t : TravelerList::allusers)
	{	if (!first) sqlfile << ',';
		first = 0;
		// look for this traveler in the listinfo map
		auto it = TravelerList::listinfo.find(t.traveler_name);
		// if found, use the description and includeInRanks values from the map
		if (it != TravelerList::listinfo.end())
		{	sqlfile << "('" << t.traveler_name << "','" << double_quotes(it->second[0]) << "'," << (it->second[1] == "1" ? "TRUE" : "FALSE") << ")\n";
		    // remove it from the map
			TravelerList::listinfo.erase(it);
		}
		else 
		{   // use the defaults
			sqlfile << "('" << t.traveler_name << "','" << double_quotes(TravelerList::defaults[0]) << "'," << (TravelerList::defaults[1] == "1" ? "TRUE" : "FALSE") << ")\n";
		}
	}
	sqlfile << ";\n";

	// report any remaining entries in the listinfo map as errors (TODO)

	// create indexes for some tables to improve query performance
	sqlfile << "DROP INDEX IF EXISTS idx_com_region_traveler ON clinchedOverallMileageByRegion;\n";
	sqlfile << "DROP INDEX IF EXISTS idx_le_traveler_includeInRanks ON listEntries;\n"; 
	sqlfile << "DROP INDEX IF EXISTS idx_routes_region_systemName ON routes;\n";
	sqlfile << "DROP INDEX IF EXISTS idx_cr_route_traveler ON clinchedRoutes;\n";
	sqlfile << "DROP INDEX IF EXISTS idx_systems_systemName ON systems;\n";
	sqlfile << "CREATE INDEX idx_com_region_traveler ON clinchedOverallMileageByRegion (region, traveler);\n";
	sqlfile << "CREATE INDEX idx_le_traveler_includeInRanks ON listEntries (traveler, includeInRanks);\n"; 
	sqlfile << "CREATE INDEX idx_routes_region_systemName ON routes (region, systemName);\n";
	sqlfile << "CREATE INDEX idx_cr_route_traveler ON clinchedRoutes (route, traveler);\n";
	sqlfile << "CREATE INDEX idx_systems_systemName ON systems (systemName);\n";

	// updates entries
      #ifndef threading_enabled
	std::cout << et->et() << "...updates" << std::endl;
      #endif
	sqlfile << "CREATE TABLE updates (date VARCHAR(" << DBFieldLength::date
		<< "), region VARCHAR(" << DBFieldLength::countryRegion
		<< "), route VARCHAR(" << DBFieldLength::routeLongName
		<< "), root VARCHAR(" << DBFieldLength::root
		<< "), description VARCHAR(" << DBFieldLength::updateText
		<< "));\n";
	if (updates->size())
	{	sqlfile << "INSERT INTO updates VALUES\n";
		first = 1;
		for (std::string* &update : *updates)
		{	if (!first) sqlfile << ',';
			first = 0;
			sqlfile << "('"  << update[0] << "','" << double_quotes(update[1]) << "','" << double_quotes(update[2])
				<< "','" << update[3] << "','" << double_quotes(update[4]) << "')\n";
			delete[] update;
		}
		sqlfile << ";\n";
	}

	// systemUpdates entries
      #ifndef threading_enabled
	std::cout << et->et() << "...systemUpdates" << std::endl;
      #endif
	sqlfile << "CREATE TABLE systemUpdates (date VARCHAR(" << DBFieldLength::date
		<< "), region VARCHAR(" << DBFieldLength::countryRegion
		<< "), systemName VARCHAR(" << DBFieldLength::systemName
		<< "), description VARCHAR(" << DBFieldLength::systemFullName
		<< "), statusChange VARCHAR(" << DBFieldLength::statusChange
		<< "));\n";
	if (systemupdates->size())
	{	sqlfile << "INSERT INTO systemUpdates VALUES\n";
		first = 1;
		for (std::string* &systemupdate : *systemupdates)
		{	if (!first) sqlfile << ',';
			first = 0;
			sqlfile << "('"  << systemupdate[0] << "','" << double_quotes(systemupdate[1])
				<< "','" << systemupdate[2] << "','" << double_quotes(systemupdate[3]) << "','" << systemupdate[4] << "')\n";
			delete[] systemupdate;
		}
		sqlfile << ";\n";
	}

	// datacheck errors into the db
      #ifndef threading_enabled
	std::cout << et->et() << "...datacheckErrors" << std::endl;
      #endif
	sqlfile << "CREATE TABLE datacheckErrors (route VARCHAR(" << DBFieldLength::root
		<< "), label1 VARCHAR(" << DBFieldLength::label
		<< "), label2 VARCHAR(" << DBFieldLength::label
		<< "), label3 VARCHAR(" << DBFieldLength::label
		<< "), code VARCHAR(" << DBFieldLength::dcErrCode
		<< "), value VARCHAR(" << DBFieldLength::dcErrValue
		<< "), falsePositive BOOLEAN, FOREIGN KEY (route) REFERENCES routes(root));\n";
	if (Datacheck::errors.size())
	{	sqlfile << "INSERT INTO datacheckErrors VALUES\n";
		bool first = 1;
		for (Datacheck &d : Datacheck::errors)
		{	if (!first) sqlfile << ',';
			first = 0;
			sqlfile << "('" << d.route->root << "',";
			sqlfile << "'"  << d.label1 << "','" << d.label2 << "','" << d.label3 << "',";
			sqlfile << "'"  << d.code << "','" << d.info << "','" << int(d.fp) << "')\n";
		}
	}
	sqlfile << ";\n";
	sqlfile.close();
      #ifdef threading_enabled
	term_mtx->lock();
	std::cout << '\n' << et->et() << "Pause writing database file " << Args::databasename << ".sql.\n" << std::flush;
	term_mtx->unlock();
      #endif
     }

void sqlfile2(ElapsedTime *et, std::list<std::array<std::string,3>> *graph_types)
{	std::ofstream sqlfile(Args::databasename+".sql", std::ios::app);

	// update graph info in DB if graphs were generated
	if (!Args::skipgraphs)
	{
	      #ifndef threading_enabled
		std::cout << et->et() << "...graphs" << std::endl;
	      #endif
		sqlfile << "DROP TABLE IF EXISTS graphArchives;\n";
		sqlfile << "DROP TABLE IF EXISTS graphArchiveSets;\n";
		sqlfile << "DROP TABLE IF EXISTS graphs;\n";
		sqlfile << "DROP TABLE IF EXISTS graphTypes;\n";
		sqlfile << "CREATE TABLE graphTypes (category VARCHAR(" << DBFieldLength::graphCategory
			<< "), descr VARCHAR(" << DBFieldLength::graphDescr
			<< "), longDescr TEXT, PRIMARY KEY(category));\n";
		if (graph_types->size())
		{	sqlfile << "INSERT INTO graphTypes VALUES\n";
			bool first = 1;
			for (std::array<std::string,3> &g : *graph_types)
			{	if (!first) sqlfile << ',';
				first = 0;
				sqlfile << "('" << g[0] << "','" << g[1] << "','" << g[2] << "')\n";
			}
			sqlfile << ";\n";
		}
		sqlfile << "CREATE TABLE graphs (filename VARCHAR(" << DBFieldLength::graphFilename
			<< "), descr VARCHAR(" << DBFieldLength::graphDescr
			<< "), vertices INTEGER, edges INTEGER, travelers INTEGER, "
			<< "format VARCHAR(" << DBFieldLength::graphFormat
			<< "), category VARCHAR(" << DBFieldLength::graphCategory
			<< "), FOREIGN KEY (category) REFERENCES graphTypes(category));\n";
		if (GraphListEntry::entries.size())
		{	sqlfile << "INSERT INTO graphs VALUES\n";
			for (size_t g = 0; g < GraphListEntry::entries.size(); g++)
			{	if (g) sqlfile << ',';
				#define G GraphListEntry::entries[g]
				sqlfile << "('"  << G.filename() << "','" << double_quotes(G.descr)
					<< "','" << G.vertices   << "','" << G.edges
					<< "','" << G.travelers  << "','" << G.format()
					<< "','" << G.category() << "')\n";
				#undef G
			}
			sqlfile << ";\n";
		}
		sqlfile << "CREATE TABLE graphArchiveSets (setName VARCHAR("
			<< DBFieldLength::setName << "), descr VARCHAR("
			<< DBFieldLength::graphDescr
			<< "), dateStamp DATE, hwyDataVers VARCHAR("
			<< DBFieldLength::gitCommit
			<< "), userDataVers VARCHAR("
			<< DBFieldLength::gitCommit 
			<< "), dataProcVers VARCHAR("
			<< DBFieldLength::gitCommit
			<< "), PRIMARY KEY(setName));\n";
		sqlfile << "CREATE TABLE graphArchives (filename VARCHAR("
			<< DBFieldLength::graphFilename
			<< "), descr VARCHAR("
			<< DBFieldLength::graphDescr
			<< "), vertices INTEGER, edges INTEGER, travelers INTEGER, format VARCHAR("
			<< DBFieldLength::graphFormat
			<< "), category VARCHAR("
			<< DBFieldLength::graphCategory
			<< "), setName VARCHAR("
			<< DBFieldLength::setName
			<< "), maxDegree INTEGER, avgDegree FLOAT, aspectRatio FLOAT, components INTEGER, FOREIGN KEY (category) REFERENCES graphTypes(category), FOREIGN KEY (setName) REFERENCES graphArchiveSets(setName));\n";
	}

	sqlfile.close();
}
