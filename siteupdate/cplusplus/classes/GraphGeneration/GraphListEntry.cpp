class GraphListEntry
{	/* This class encapsulates information about generated graphs for
	inclusion in the DB table.  Field names here match column names
	in the "graphs" DB table. */
	public:
	std::string filename();
	std::string descr;
	unsigned int vertices;
	unsigned int edges;
	char form;
	std::string format();
	char cat;
	std::string category();
	// additional data for the C++ version, for multithreaded subgraph writing
	std::string root;
	std::list<Region*> *regions;
	std::list<HighwaySystem*> *systems;
	PlaceRadius *placeradius;
	std::string tag();

	GraphListEntry(std::string r, std::string d, char f, char c, std::list<Region*> *rg, std::list<HighwaySystem*> *sys, PlaceRadius *pr)
	{	root = r;
		descr = d;
		form = f;
		cat = c;

		regions = rg;
		systems = sys;
		placeradius = pr;
	}
};

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
		default : return std::string("ERROR: GraphListEntry::category() unexpected category token ('")+cat+"')";
	}
}

std::string GraphListEntry::tag()
{	switch (cat)
	{	case 'a': return placeradius->base + '(' + std::to_string(placeradius->r) + ") ";
		case 'r': return regions->front()->code + ' ';			// must have valid pointer
		case 's': return systems->front()->systemname + ' ';		// must have valid pointer
		case 'S':
		case 'R': return root + ' ';
		case 'c': return regions->front()->country_code() + ' ';	// must have valid pointer
		case 'C': return regions->front()->continent_code() + ' ';	// must have valid pointer
		default : return std::string("ERROR: GraphListEntry::tag() unexpected category token ('")+cat+"')";
	}
}
