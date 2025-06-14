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
    ){
    char fstr[65];
    std::string dbdir = Args::databasepath;
    if (!dbdir.empty())
        dbdir += '/';

    // Create .sql file in the specified directory
    std::ofstream sqlfile(dbdir + Args::databasename + ".sql");
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

    int first;
    // first, continents, countries, and regions
      #ifndef threading_enabled
    std::cout << et->et() << "...continents, countries, regions" << std::endl;
      #endif
    sqlfile << "CREATE TABLE continents (code VARCHAR(" << DBFieldLength::continentCode
        << "), name VARCHAR(" << DBFieldLength::continentName
        << "), PRIMARY KEY(code));\n";

    // Write continents data to CSV
    std::ofstream continents_csv(dbdir + "continents.csv");
    for (size_t c = 0; c < Region::continents.size()-1; c++)
    {
        continents_csv << Region::continents[c].first << "," << Region::continents[c].second << "\n";
    }
    continents_csv.close();

    // Use LOAD DATA to import continents from CSV (no path in SQL)
    sqlfile << "LOAD DATA LOCAL INFILE 'continents.csv' INTO TABLE continents FIELDS TERMINATED BY ',' LINES TERMINATED BY '\\n' (code, name);\n";

    sqlfile << "CREATE TABLE countries (code VARCHAR(" << DBFieldLength::countryCode
        << "), name VARCHAR(" << DBFieldLength::countryName
        << "), PRIMARY KEY(code));\n";

    // Write countries data to CSV
    std::ofstream countries_csv(dbdir + "countries.csv");
    for (size_t c = 0; c < Region::countries.size()-1; c++)
    {
        countries_csv << Region::countries[c].first << "," << double_quotes(Region::countries[c].second) << "\n";
    }
    countries_csv.close();

    // Use LOAD DATA to import countries from CSV (no path in SQL)
    sqlfile << "LOAD DATA LOCAL INFILE 'countries.csv' INTO TABLE countries FIELDS TERMINATED BY ',' LINES TERMINATED BY '\\n' (code, name);\n";

    sqlfile << "CREATE TABLE regions (code VARCHAR(" << DBFieldLength::regionCode
        << "), name VARCHAR(" << DBFieldLength::regionName
        << "), country VARCHAR(" << DBFieldLength::countryCode
        << "), continent VARCHAR(" << DBFieldLength::continentCode
        << "), regiontype VARCHAR(" << DBFieldLength::regiontype
        << "), ";
    sqlfile << "PRIMARY KEY(code), FOREIGN KEY (country) REFERENCES countries(code), FOREIGN KEY (continent) REFERENCES continents(code));\n";

    // Write regions data to CSV
    std::ofstream regions_csv(dbdir + "regions.csv");
    for (Region *r = Region::allregions.data, *dummy = Region::allregions.end()-1; r < dummy; r++)
    {
        regions_csv << r->code << "," << double_quotes(r->name)
            << "," << r->country_code() << "," << r->continent_code()
            << "," << r->type << "\n";
    }
    regions_csv.close();

    // Use LOAD DATA to import regions from CSV (no path in SQL)
    sqlfile << "LOAD DATA LOCAL INFILE 'regions.csv' INTO TABLE regions FIELDS TERMINATED BY ',' LINES TERMINATED BY '\\n' (code, name, country, continent, regiontype);\n";

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

    // Write systems data to CSV
    std::ofstream systems_csv(dbdir + "systems.csv");
    unsigned int csvOrder = 0;
    for (HighwaySystem& h : HighwaySystem::syslist)
    {
        systems_csv << h.systemname << "," << h.country->first << ","
            << double_quotes(h.fullname) << "," << h.color << "," << h.level_name()
            << "," << h.tier << "," << csvOrder << "\n";
        csvOrder += 1;
    }
    systems_csv.close();

    // Use LOAD DATA to import systems from CSV
    sqlfile << "LOAD DATA LOCAL INFILE 'systems.csv' INTO TABLE systems FIELDS TERMINATED BY ',' LINES TERMINATED BY '\\n' (systemName, countryCode, fullName, color, level, tier, csvOrder);\n";

    // --- routes table ---
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

    // Write routes data to CSV
    std::ofstream routes_csv(dbdir + "routes.csv");
    csvOrder = 0;
    for (HighwaySystem& h : HighwaySystem::syslist)
      for (Route& r : h.routes)
      {
        *fmt::format_to(fstr, "{}", r.mileage) = 0;
        routes_csv << r.system->systemname << "," << r.region->code << "," << r.route << "," << r.banner << "," << r.abbrev
            << "," << double_quotes(r.city) << "," << r.root << "," << fstr << "," << r.rootOrder << "," << csvOrder << "\n";
        csvOrder += 1;
      }
    routes_csv.close();

    // Use LOAD DATA to import routes from CSV
    sqlfile << "LOAD DATA LOCAL INFILE 'routes.csv' INTO TABLE routes FIELDS TERMINATED BY ',' LINES TERMINATED BY '\\n' (systemName, region, route, banner, abbrev, city, root, mileage, rootOrder, csvOrder);\n";

    // --- connectedRoutes table ---
    #ifndef threading_enabled
    std::cout << et->et() << "...connectedRoutes" << std::endl;
    #endif
    sqlfile << "CREATE TABLE connectedRoutes (systemName VARCHAR(" << DBFieldLength::systemName
        << "), route VARCHAR(" << DBFieldLength::route
        << "), banner VARCHAR(" << DBFieldLength::banner
        << "), groupName VARCHAR(" << DBFieldLength::city
        << "), firstRoot VARCHAR(" << DBFieldLength::root
        << "), mileage FLOAT, csvOrder INTEGER, PRIMARY KEY(firstRoot), FOREIGN KEY (firstRoot) REFERENCES routes(root));\n";

    // Write connectedRoutes data to CSV
    std::ofstream conroutes_csv(dbdir + "connectedRoutes.csv");
    csvOrder = 0;
    for (HighwaySystem& h : HighwaySystem::syslist)
      for (ConnectedRoute& cr : h.con_routes)
      {
        *fmt::format_to(fstr, "{}", cr.mileage) = 0;
        conroutes_csv << cr.system->systemname << "," << cr.route << "," << cr.banner << "," << double_quotes(cr.groupname)
            << "," << (cr.roots.size() ? cr.roots[0]->root.data() : "ERROR_NO_ROOTS") << "," << fstr << "," << csvOrder << "\n";
        csvOrder += 1;
      }
    conroutes_csv.close();

    // Use LOAD DATA to import connectedRoutes from CSV
    sqlfile << "LOAD DATA LOCAL INFILE 'connectedRoutes.csv' INTO TABLE connectedRoutes FIELDS TERMINATED BY ',' LINES TERMINATED BY '\\n' (systemName, route, banner, groupName, firstRoot, mileage, csvOrder);\n";

    // --- connectedRouteRoots table ---
    #ifndef threading_enabled
    std::cout << et->et() << "...connectedRouteRoots" << std::endl;
    #endif
    sqlfile << "CREATE TABLE connectedRouteRoots (firstRoot VARCHAR(" << DBFieldLength::root
        << "), root VARCHAR(" << DBFieldLength::root
        << "), FOREIGN KEY (firstRoot) REFERENCES connectedRoutes(firstRoot));\n";

    // Write connectedRouteRoots data to CSV
    std::ofstream conroots_csv(dbdir + "connectedRouteRoots.csv");
    for (HighwaySystem& h : HighwaySystem::syslist)
      for (ConnectedRoute& cr : h.con_routes)
        for (unsigned int i = 1; i < cr.roots.size(); i++)
        {
            conroots_csv << cr.roots[0]->root << "," << cr.roots[i]->root << "\n";
        }
    conroots_csv.close();

    // Use LOAD DATA to import connectedRouteRoots from CSV
    sqlfile << "LOAD DATA LOCAL INFILE 'connectedRouteRoots.csv' INTO TABLE connectedRouteRoots FIELDS TERMINATED BY ',' LINES TERMINATED BY '\\n' (firstRoot, root);\n";

    // --- waypoints table ---
    #ifndef threading_enabled
    std::cout << et->et() << "...waypoints" << std::endl;
    #endif
    sqlfile << "CREATE TABLE waypoints (pointId INTEGER, pointName VARCHAR(" << DBFieldLength::label
        << "), latitude DOUBLE, longitude DOUBLE, root VARCHAR(" << DBFieldLength::root
        << "), PRIMARY KEY(pointId), FOREIGN KEY (root) REFERENCES routes(root));\n";

    // Write waypoints data to CSV
    std::ofstream waypoints_csv(dbdir + "waypoints.csv");
    unsigned int point_num = 0;
    for (HighwaySystem& h : HighwaySystem::syslist)
      for (Route& r : h.routes)
        for (Waypoint& w : r.points)
        {
            *fmt::format_to(fstr,"{:.15}",w.lat) = 0;
            std::string lat_str = fstr;
            if (w.lat==int(w.lat)) lat_str += ".0";
            *fmt::format_to(fstr,"{:.15}",w.lng) = 0;
            std::string lng_str = fstr;
            if (w.lng==int(w.lng)) lng_str += ".0";
            waypoints_csv << point_num << "," << w.label << "," << lat_str << "," << lng_str << "," << r.root << "\n";
            point_num += 1;
        }
    waypoints_csv.close();

    // Use LOAD DATA to import waypoints from CSV
    sqlfile << "LOAD DATA LOCAL INFILE 'waypoints.csv' INTO TABLE waypoints FIELDS TERMINATED BY ',' LINES TERMINATED BY '\\n' (pointId, pointName, latitude, longitude, root);\n";

    // --- segments table ---
    #ifndef threading_enabled
    std::cout << et->et() << "...segments" << std::endl;
    #endif
    sqlfile << "CREATE TABLE segments (segmentId INTEGER, waypoint1 INTEGER, waypoint2 INTEGER, root VARCHAR(" << DBFieldLength::root
        << "), PRIMARY KEY (segmentId), FOREIGN KEY (waypoint1) REFERENCES waypoints(pointId), "
        << "FOREIGN KEY (waypoint2) REFERENCES waypoints(pointId), FOREIGN KEY (root) REFERENCES routes(root));\n";

    // Write segments data to CSV
    std::ofstream segments_csv(dbdir + "segments.csv");
    unsigned int segment_num = 0;
    point_num = 0;
    std::vector<std::pair<unsigned int, const char*>> clinched_list;
    for (HighwaySystem& h : HighwaySystem::syslist)
      for (Route& r : h.routes)
        for (HighwaySegment& s : r.segments)
        {
            segments_csv << segment_num << "," << point_num << "," << point_num+1 << "," << r.root << "\n";
            for (TravelerList *t : s.clinched_by)
                clinched_list.emplace_back(segment_num, t->traveler_name.data());
            segment_num += 1;
            point_num++;
        }
    segments_csv.close();

    // Use LOAD DATA to import segments from CSV
    sqlfile << "LOAD DATA LOCAL INFILE 'segments.csv' INTO TABLE segments FIELDS TERMINATED BY ',' LINES TERMINATED BY '\\n' (segmentId, waypoint1, waypoint2, root);\n";

    // --- clinched table ---
    #ifndef threading_enabled
    std::cout << et->et() << "...clinched" << std::endl;
    #endif
    sqlfile << "CREATE TABLE clinched (segmentId INTEGER, traveler VARCHAR(" << DBFieldLength::traveler
        << "), FOREIGN KEY (segmentId) REFERENCES segments(segmentId));\n";

    // Write clinched data to CSV
    std::ofstream clinched_csv(dbdir + "clinched.csv");
    for (const auto& entry : clinched_list)
    {
        clinched_csv << entry.first << "," << entry.second << "\n";
    }
    clinched_csv.close();

    // Use LOAD DATA to import clinched from CSV
    sqlfile << "LOAD DATA LOCAL INFILE 'clinched.csv' INTO TABLE clinched FIELDS TERMINATED BY ',' LINES TERMINATED BY '\\n' (segmentId, traveler);\n";

    // --- overallMileageByRegion table ---
    #ifndef threading_enabled
    std::cout << et->et() << "...overallMileageByRegion" << std::endl;
    #endif
    sqlfile << "CREATE TABLE overallMileageByRegion (region VARCHAR(" << DBFieldLength::regionCode
        << "), activeMileage DOUBLE, activePreviewMileage DOUBLE);\n";

    // Write overallMileageByRegion data to CSV
    std::ofstream ombr_csv(dbdir + "overallMileageByRegion.csv");
    for (Region& region : Region::allregions)
    {
        if (region.active_only_mileage+region.active_preview_mileage == 0) continue;
        *fmt::format_to(fstr, "{},{}", region.active_only_mileage, region.active_preview_mileage) = 0;
        ombr_csv << region.code << "," << fstr << "\n";
    }
    ombr_csv.close();

    // Use LOAD DATA to import overallMileageByRegion from CSV
    sqlfile << "LOAD DATA LOCAL INFILE 'overallMileageByRegion.csv' INTO TABLE overallMileageByRegion FIELDS TERMINATED BY ',' LINES TERMINATED BY '\\n' (region, activeMileage, activePreviewMileage);\n";

    // --- systemMileageByRegion table ---
    #ifndef threading_enabled
    std::cout << et->et() << "...systemMileageByRegion" << std::endl;
    #endif
    sqlfile << "CREATE TABLE systemMileageByRegion (systemName VARCHAR(" << DBFieldLength::systemName
        << "), region VARCHAR(" << DBFieldLength::regionCode
        << "), mileage DOUBLE, FOREIGN KEY (systemName) REFERENCES systems(systemName));\n";

    // Write systemMileageByRegion data to CSV
    std::ofstream smbr_csv(dbdir + "systemMileageByRegion.csv");
    for (HighwaySystem& h : HighwaySystem::syslist)
      if (h.active_or_preview())
        for (std::pair<Region* const,double>& rm : h.mileage_by_region)
        {
            *fmt::format_to(fstr, "{}", rm.second) = 0;
            smbr_csv << h.systemname << "," << rm.first->code << "," << fstr << "\n";
        }
    smbr_csv.close();

    // Use LOAD DATA to import systemMileageByRegion from CSV
    sqlfile << "LOAD DATA LOCAL INFILE 'systemMileageByRegion.csv' INTO TABLE systemMileageByRegion FIELDS TERMINATED BY ',' LINES TERMINATED BY '\\n' (systemName, region, mileage);\n";

    // --- clinchedOverallMileageByRegion table ---
    #ifndef threading_enabled
    std::cout << et->et() << "...clinchedOverallMileageByRegion" << std::endl;
    #endif
    sqlfile << "CREATE TABLE clinchedOverallMileageByRegion (region VARCHAR(" << DBFieldLength::regionCode
        << "), traveler VARCHAR(" << DBFieldLength::traveler
        << "), activeMileage DOUBLE, activePreviewMileage DOUBLE);\n";

    // Write clinchedOverallMileageByRegion data to CSV
    std::ofstream combr_csv(dbdir + "clinchedOverallMileageByRegion.csv");
    for (TravelerList& t : TravelerList::allusers)
      for (std::pair<Region* const,double>& rm : t.active_preview_mileage_by_region)
      {
        auto it = t.active_only_mileage_by_region.find(rm.first);
        double active_miles = (it != t.active_only_mileage_by_region.end()) ? it->second : 0;
        *fmt::format_to(fstr, "{},{}", active_miles, rm.second) = 0;
        combr_csv << rm.first->code << "," << t.traveler_name << "," << fstr << "\n";
      }
    combr_csv.close();

    // Use LOAD DATA LOCAL INFILE to import clinchedOverallMileageByRegion from CSV
    sqlfile << "LOAD DATA LOCAL INFILE 'clinchedOverallMileageByRegion.csv' INTO TABLE clinchedOverallMileageByRegion FIELDS TERMINATED BY ',' LINES TERMINATED BY '\\n' (region, traveler, activeMileage, activePreviewMileage);\n";

    // --- clinchedSystemMileageByRegion table ---
    #ifndef threading_enabled
    std::cout << et->et() << "...clinchedSystemMileageByRegion" << std::endl;
    #endif
    sqlfile << "CREATE TABLE clinchedSystemMileageByRegion (systemName VARCHAR(" << DBFieldLength::systemName
        << "), region VARCHAR(" << DBFieldLength::regionCode
        << "), traveler VARCHAR(" << DBFieldLength::traveler
        << "), mileage DOUBLE, FOREIGN KEY (systemName) REFERENCES systems(systemName));\n";

    // Write clinchedSystemMileageByRegion data to CSV
    std::ofstream csmbr_csv(dbdir + "clinchedSystemMileageByRegion.csv");
    for (TravelerList& t : TravelerList::allusers)
      for (auto& csmbr : t.system_region_mileages)
      {
        auto& systemname = csmbr.first->systemname;
        for (auto& rm : csmbr.second)
        {
            *fmt::format_to(fstr, "{}", rm.second) = 0;
            csmbr_csv << systemname << "," << rm.first->code << "," << t.traveler_name << "," << fstr << "\n";
        }
      }
    csmbr_csv.close();

    // Use LOAD DATA to import clinchedSystemMileageByRegion from CSV
    sqlfile << "LOAD DATA LOCAL INFILE 'clinchedSystemMileageByRegion.csv' INTO TABLE clinchedSystemMileageByRegion FIELDS TERMINATED BY ',' LINES TERMINATED BY '\\n' (systemName, region, traveler, mileage);\n";

    // --- clinchedConnectedRoutes table ---
    #ifndef threading_enabled
    std::cout << et->et() << "...clinchedConnectedRoutes" << std::endl;
    #endif
    sqlfile << "CREATE TABLE clinchedConnectedRoutes (route VARCHAR(" << DBFieldLength::root
        << "), traveler VARCHAR(" << DBFieldLength::traveler
        << "), mileage FLOAT, clinched BOOLEAN, FOREIGN KEY (route) REFERENCES connectedRoutes(firstRoot));\n";

    // Write clinchedConnectedRoutes data to CSV
    std::ofstream ccr_csv(dbdir + "clinchedConnectedRoutes.csv");
    for (TravelerList& t : TravelerList::allusers)
      for (auto& rm : t.ccr_values)
      {
        *fmt::format_to(fstr, "{}", rm.second) = 0;
        ccr_csv << rm.first->roots[0]->root << "," << t.traveler_name << "," << fstr << "," << (rm.second == rm.first->mileage) << "\n";
      }
    ccr_csv.close();

    // Use LOAD DATA to import clinchedConnectedRoutes from CSV
    sqlfile << "LOAD DATA LOCAL INFILE 'clinchedConnectedRoutes.csv' INTO TABLE clinchedConnectedRoutes FIELDS TERMINATED BY ',' LINES TERMINATED BY '\\n' (route, traveler, mileage, clinched);\n";

    // --- clinchedRoutes table ---
    #ifndef threading_enabled
    std::cout << et->et() << "...clinchedRoutes" << std::endl;
    #endif
    sqlfile << "CREATE TABLE clinchedRoutes (route VARCHAR(" << DBFieldLength::root
        << "), traveler VARCHAR(" << DBFieldLength::traveler
        << "), mileage FLOAT, clinched BOOLEAN, FOREIGN KEY (route) REFERENCES routes(root));\n";

    // Write clinchedRoutes data to CSV
    std::ofstream cr_csv(dbdir + "clinchedRoutes.csv");
    for (TravelerList& t : TravelerList::allusers)
      for (std::pair<Route*,double>& rm : t.cr_values)
      {
        *fmt::format_to(fstr, "{}", rm.second) = 0;
        cr_csv << rm.first->root << "," << t.traveler_name << "," << fstr << "," << (rm.second >= rm.first->mileage) << "\n";
      }
    cr_csv.close();

    // Use LOAD DATA to import clinchedRoutes from CSV
    sqlfile << "LOAD DATA LOCAL INFILE 'clinchedRoutes.csv' INTO TABLE clinchedRoutes FIELDS TERMINATED BY ',' LINES TERMINATED BY '\\n' (route, traveler, mileage, clinched);\n";

    // --- listEntries table ---
    #ifndef threading_enabled
    std::cout << et->et() << "...listEntries" << std::endl;
    #endif
    sqlfile << "CREATE TABLE listEntries (traveler VARCHAR(" << DBFieldLength::traveler
        << "), description VARCHAR(" << DBFieldLength::listDescription
        << "), includeInRanks BOOLEAN);\n";

    // Write listEntries data to CSV
    std::ofstream listentries_csv(dbdir + "listEntries.csv");
    for (TravelerList& t : TravelerList::allusers)
    {
        // look for this traveler in the listinfo map
        auto it = TravelerList::listinfo.find(t.traveler_name);
        // if found, use the description and includeInRanks values from the map
        if (it != TravelerList::listinfo.end())
        {
            listentries_csv << t.traveler_name << "," << double_quotes(it->second[0]) << "," << (it->second[1] == "1" ? "TRUE" : "FALSE") << "\n";
            // remove it from the map
            TravelerList::listinfo.erase(it);
        }
        else
        {   // use the defaults
            listentries_csv << t.traveler_name << "," << double_quotes(TravelerList::defaults[0]) << "," << (TravelerList::defaults[1] == "1" ? "TRUE" : "FALSE") << "\n";
        }
    }
    listentries_csv.close();

    // Use LOAD DATA to import listEntries from CSV
    sqlfile << "LOAD DATA LOCAL INFILE 'listEntries.csv' INTO TABLE listEntries FIELDS TERMINATED BY ',' LINES TERMINATED BY '\\n' (traveler, description, includeInRanks);\n";

    // --- datacheckErrors table ---
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

    // Write datacheckErrors data to CSV
    std::ofstream dce_csv(dbdir + "datacheckErrors.csv");
    for (Datacheck &d : Datacheck::errors)
    {
        dce_csv << d.route->root << "," << d.label1 << "," << d.label2 << "," << d.label3 << ","
                << d.code << "," << d.info << "," << int(d.fp) << "\n";
    }
    dce_csv.close();

    // Use LOAD DATA to import datacheckErrors from CSV
    sqlfile << "LOAD DATA LOCAL INFILE 'datacheckErrors.csv' INTO TABLE datacheckErrors FIELDS TERMINATED BY ',' LINES TERMINATED BY '\\n' (route, label1, label2, label3, code, value, falsePositive);\n";

    // create indexes for some tables to improve query performance
	sqlfile << "DROP INDEX IF EXISTS idx_com_region_traveler ON clinchedOverallMileageByRegion;\n";
	sqlfile << "DROP INDEX IF EXISTS idx_le_traveler_includeInRanks ON listEntries;\n"; 
	sqlfile << "DROP INDEX IF EXISTS idx_routes_region_systemName ON routes;\n";
	sqlfile << "DROP INDEX IF EXISTS idx_cr_route_traveler ON clinchedRoutes;\n";
	sqlfile << "DROP INDEX IF EXISTS idx_systems_systemName ON systems;\n";
	sqlfile << "DROP INDEX IF EXISTS idx_waypoints_lat_lng ON waypoints;\n";
	sqlfile << "DROP INDEX IF EXISTS idx_clinched_segment_traveler ON clinched;\n";
	sqlfile << "CREATE INDEX idx_com_region_traveler ON clinchedOverallMileageByRegion (region, traveler);\n";
	sqlfile << "CREATE INDEX idx_le_traveler_includeInRanks ON listEntries (traveler, includeInRanks);\n"; 
	sqlfile << "CREATE INDEX idx_routes_region_systemName ON routes (region, systemName);\n";
	sqlfile << "CREATE INDEX idx_cr_route_traveler ON clinchedRoutes (route, traveler);\n";
	sqlfile << "CREATE INDEX idx_systems_systemName ON systems (systemName);\n";
	sqlfile << "CREATE INDEX idx_waypoints_lat_lng ON waypoints (latitude, longitude);\n";
	sqlfile << "CREATE INDEX idx_clinched_segment_traveler ON clinched (segmentId, traveler);\n";

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

    // Write updates data to CSV
    std::ofstream updates_csv(dbdir + "updates.csv");
    for (std::string* &update : *updates)
    {
        updates_csv << update[0] << "," << double_quotes(update[1]) << "," << double_quotes(update[2])
            << "," << update[3] << "," << double_quotes(update[4]) << "\n";
        delete[] update;
    }
    updates_csv.close();

    // Use LOAD DATA to import updates from CSV
    sqlfile << "LOAD DATA LOCAL INFILE 'updates.csv' INTO TABLE updates FIELDS TERMINATED BY ',' LINES TERMINATED BY '\\n' (date, region, route, root, description);\n";

    // --- systemUpdates table ---
    #ifndef threading_enabled
    std::cout << et->et() << "...systemUpdates" << std::endl;
    #endif
    sqlfile << "CREATE TABLE systemUpdates (date VARCHAR(" << DBFieldLength::date
        << "), region VARCHAR(" << DBFieldLength::countryRegion
        << "), systemName VARCHAR(" << DBFieldLength::systemName
        << "), description VARCHAR(" << DBFieldLength::systemFullName
        << "), statusChange VARCHAR(" << DBFieldLength::statusChange
        << "));\n";

    // Write systemUpdates data to CSV
    std::ofstream systemupdates_csv(dbdir + "systemUpdates.csv");
    for (std::string* &systemupdate : *systemupdates)
    {
        systemupdates_csv << systemupdate[0] << "," << double_quotes(systemupdate[1])
            << "," << systemupdate[2] << "," << double_quotes(systemupdate[3]) << "," << systemupdate[4] << "\n";
        delete[] systemupdate;
    }
    systemupdates_csv.close();

    // Use LOAD DATA to import systemUpdates from CSV
    sqlfile << "LOAD DATA LOCAL INFILE 'systemUpdates.csv' INTO TABLE systemUpdates FIELDS TERMINATED BY ',' LINES TERMINATED BY '\\n' (date, region, systemName, description, statusChange);\n";

	sqlfile << ";\n";
	sqlfile.close();
      #ifdef threading_enabled
	term_mtx->lock();
	std::cout << '\n' << et->et() << "Pause writing database file " << Args::databasename << ".sql.\n" << std::flush;
	term_mtx->unlock();
      #endif
     }

void sqlfile2(ElapsedTime *et, std::list<std::array<std::string,3>> *graph_types)
{
    std::string dbdir = Args::databasepath;
    if (!dbdir.empty())
        dbdir += '/';
    std::ofstream sqlfile(dbdir + Args::databasename + ".sql", std::ios::app);

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
