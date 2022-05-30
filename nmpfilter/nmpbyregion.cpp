// Travel Mapping Project, Eric Bryant, 2018-2022
#include <cstring>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <vector>
using namespace std;

struct Region
{	vector<string> lines;
	unsigned int unmarked;
	unsigned int fps;

	static unsigned int master_unmarked;
	static unsigned int master_fps;

	Region(): unmarked(0), fps(0) {}

	bool operator != (const Region& other)
	{	return this != &other;
	}

	void add_info(string& w1, string& w2, bool count_master)
	{	lines.push_back(w1);
		lines.push_back(w2);
		if (strcmp(w1.data()+w1.size()-2, "FP") && strcmp(w1.data()+w1.size()-4, "FPLI"))
		     {	unmarked++;
			if (count_master) master_unmarked++;
		     }
		else {	fps++;
			if (count_master) master_fps++;
		     }
	}
};
unsigned int Region::master_unmarked = 0;
unsigned int Region::master_fps = 0;

bool descending_by_unmarked_count(pair<const string,Region>* a, pair<const string,Region>* b)
{	return a->second.unmarked > b->second.unmarked;
}

int main(int argc, char *argv[])
{	if (argc != 3)
	{	cout << "usage: nmpbyregion MasterNMP OutputDir\n";
		return 0;
	}

	ifstream master(argv[1]);
	if (!master)
	{	std::cout << argv[1] << " not found\n";
		return 1;
	}

	// read tm-master.nmp and populate region data
	map<string, Region> region_map;
	string w1, w2;
	while (getline(master, w1) && getline(master, w2))
	{	Region& r1 = region_map[w1.substr(0, w1.find('.'))];
		Region& r2 = region_map[w2.substr(0, w2.find('.'))];
		r1.add_info(w1, w2, 1);
		if (r1 != r2) r2.add_info(w1, w2, 0);
	}

	// sort regions by number of unmarked NMP pairs
	list<pair<const string,Region>*> region_list;
	for (auto& r : region_map) region_list.push_back(&r);
	region_list.sort(descending_by_unmarked_count);

	// index.html table header & tm-master entry
	ofstream html(string(argv[2])+"/index.html");
	html << "<h1>NMP by Region</h1>\n";
	html << "<table border=1 cellpadding=2 cellspacing=0>";
	html << "<tr><td><b>file</b></td><td><b>unmarked pairs</b></td><td><b>pairs marked FP</b></td><td><b>total NMP pairs</b></td><td><b>links</b></td></tr>\n";
	html << "<tr><td>tm-master.nmp</td><td>"
	     << Region::master_unmarked << "</td><td>"
	     << Region::master_fps << "</td><td>"
	     << Region::master_unmarked + Region::master_fps << "</td><td>"
	     << "<a href='../tm-master.nmp'>file</a> "
	     << "<a href='https://courses.teresco.org/metal/hdx?load=tm-master.nmp'>HDX</a></td></tr>\n";

	// write .nmp files & table entries
	for (auto& r : region_list)
	{	ofstream nmp(argv[2]+('/'+r->first)+".nmp");
		for (string& line : r->second.lines) nmp << line << '\n';
		html << "<tr><td>" << r->first << ".nmp</td>";
		html << "<td>" << r->second.unmarked << "</td>";
		html << "<td>" << r->second.fps << "</td>";
		html << "<td>" << r->second.fps + r->second.unmarked << "</td><td>";
		html << "<a href='" << r->first << ".nmp'>file</a> ";
		html << "<a href='https://courses.teresco.org/metal/hdx?load=" << r->first << ".nmp'>HDX</a></td></tr>\n";
	}
	html << "</table>";
}
