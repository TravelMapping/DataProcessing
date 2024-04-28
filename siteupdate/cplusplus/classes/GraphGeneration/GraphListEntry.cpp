#include "GraphListEntry.h"
#include "PlaceRadius.h"
#include "../HighwaySystem/HighwaySystem.h"
#include "../Region/Region.h"

std::vector<GraphListEntry> GraphListEntry::entries;
size_t GraphListEntry::num; // iterator for entries

GraphListEntry::GraphListEntry(std::string r, std::string d, char f, char c, std::vector<Region*> *rg, std::vector<HighwaySystem*> *sys, PlaceRadius *pr):
	regions(rg), systems(sys), placeradius(pr),
	root(r), descr(d), form(f), cat(c) {}

void GraphListEntry::add_group(std::string&& r, std::string&& d, char c, std::vector<Region*> *rg, std::vector<HighwaySystem*> *sys, PlaceRadius *pr)
{	GraphListEntry::entries.emplace_back(r, d, 's', c, rg, sys, pr);
	GraphListEntry::entries.emplace_back(r, d, 'c', c, rg, sys, pr);
	GraphListEntry::entries.emplace_back(r, d, 't', c, rg, sys, pr);
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
	{	case 'a': char fstr[51];
			  sprintf(fstr, "(%.15g) ", placeradius->r);
			  return placeradius->title + fstr;
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
