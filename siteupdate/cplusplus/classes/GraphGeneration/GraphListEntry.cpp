#define FMT_HEADER_ONLY
#include "GraphListEntry.h"
#include "PlaceRadius.h"
#include "../ErrorList/ErrorList.h"
#include "../HighwaySystem/HighwaySystem.h"
#include "../Region/Region.h"
#include <fmt/format.h>

std::vector<GraphListEntry> GraphListEntry::entries;
size_t GraphListEntry::num; // iterator for entries
std::unordered_map<std::string, GraphListEntry*> GraphListEntry::unique_names;

GraphListEntry::GraphListEntry(std::string r, std::string d, char f, char c, std::vector<Region*> *rg, std::vector<HighwaySystem*> *sys, PlaceRadius *pr):
	regions(rg), systems(sys), placeradius(pr),
	root(r), descr(d), form(f), cat(c) {}

void GraphListEntry::add_group(std::string&& r, std::string&& d, char c, std::vector<Region*> *rg, std::vector<HighwaySystem*> *sys, PlaceRadius *pr, ErrorList& el)
{	GraphListEntry::entries.emplace_back(r, d, 's', c, rg, sys, pr);
	GraphListEntry::entries.emplace_back(r, d, 'c', c, rg, sys, pr);
	GraphListEntry::entries.emplace_back(r, d, 't', c, rg, sys, pr);
	GraphListEntry* this_one = &GraphListEntry::entries.back();
	auto ib = unique_names.emplace(r, this_one);
	if (!ib.second)
	{	GraphListEntry* that_one = ib.first->second;
		std::string err = "Duplicate graph name " + r + " in " + that_one->category();
		if (this_one->cat != that_one->cat)
			err += " and " + this_one->category();
		err += " graphs";
		el.add_error(err);
	}
}

std::string GraphListEntry::filename()
{	switch (form)
	{	case 's': return root+"-simple.tmg";
		case 'c': return root+".tmg";
		case 't': return root+"-traveled.tmg";
		default : return std::string("ERROR: GraphListEntry::filename() unexpected format token ('")+form+"')";
	}
}

std::string GraphListEntry::format()
{	switch (form)
	{	case 's': return "simple";
		case 'c': return "collapsed";
		case 't': return "traveled";
		default : return std::string("ERROR: GraphListEntry::format() unexpected format token ('")+form+"')";
	}
}

std::string GraphListEntry::category()
{	switch (cat)
	{	case 'M': return "master";
		case 'a': return "area";
		case 'r': return "region";
		case 's': return "system";
		case 'S': return "multisystem";
		case 'R': return "multiregion";
		case 'c': return "country";
		case 'C': return "continent";
		case 'f': return "fullcustom";
		default : return std::string("ERROR: GraphListEntry::category() unexpected category token ('")+cat+"')";
	}
}

std::string GraphListEntry::tag()
{	switch (cat)
	{	case 'a': return fmt::format("{}({:.15}) ", placeradius->title, placeradius->r);
		case 'r': return regions->front()->code + ' ';			// must have valid pointer
		case 's': return systems->front()->systemname + ' ';		// must have valid pointer
		case 'S':
		case 'f':
		case 'R': return root + ' ';
		case 'c': return regions->front()->country_code() + ' ';	// must have valid pointer
		case 'C': return regions->front()->continent_code() + ' ';	// must have valid pointer
		default : return std::string("ERROR: GraphListEntry::tag() unexpected category token ('")+cat+"')";
	}
}
