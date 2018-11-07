// Travel Mapping Project, Eric Bryant, 2018
#include <ctime>
#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include <vector>
using namespace std;

class region
{	public:
	string code, name, country, continent, regionType;

	void UseRootRg()
	{	// Remove dashes from code
		for (int dash = code.find('-'); dash > 0; dash = code.find('-'))
			code.erase(dash, 1);
		// Convert to lower case
		for (unsigned int i = 0; i < code.size(); i++)
		  if (code[i] >= 'A' && code[i] <= 'Z')
		    code[i] += 32;
	}

	region (string &CSVline)
	{	unsigned int i = 0;
		while (i < CSVline.size() && CSVline[i] != ';') { code.push_back(CSVline[i]); i++; } i++;
		while (i < CSVline.size() && CSVline[i] != ';') { name.push_back(CSVline[i]); i++; } i++;
		while (i < CSVline.size() && CSVline[i] != ';') { country.push_back(CSVline[i]); i++; } i++;
		while (i < CSVline.size() && CSVline[i] != ';') { continent.push_back(CSVline[i]); i++; } i++;
		while (i < CSVline.size() && CSVline[i] != ';') { regionType.push_back(CSVline[i]); i++; } i++;
		UseRootRg();
	}
};

bool GetRegions(char *CsvFile, list<string> &CoList, vector<region> &RgList)
// Read contents of regions.csv
{	ifstream CSV(CsvFile);
	if (!CSV) { cout << CsvFile << " not found!\n"; return 0;}

	string CSVline;
	getline(CSV, CSVline); // Skip header row
	while (getline(CSV, CSVline))
	{	RgList.emplace_back(CSVline);
		CoList.emplace_back(RgList.back().country);
	}
	CoList.sort();
	list<string>::iterator c = CoList.begin();
	list<string>::iterator d = CoList.begin(); d++;
	while (d != CoList.end()) // step thru each country in list
	{	while (*c == *d) d = CoList.erase(d); // remove duplicates
		c++; d++;
	}
	return 1;
}

bool ReadMaster(char *MasterNMP, vector<string> &master)
// Read contents of tm-master.nmp
{	ifstream CSV(MasterNMP);
	if (!CSV)
	{	std::cout << MasterNMP << " not found\n";
		return 0;
	}
	string CSVline;
	while (getline(CSV, CSVline)) master.emplace_back(CSVline);
	return 1;
}

bool RgMatch(vector<region> &RgList, string &co, string rg)
// Find region code in RgList vector; check whether it matches country
{	for (unsigned int r = 0; r < RgList.size(); r++)
	{	if (RgList[r].code == rg)
			if (RgList[r].country == co) return 1;
			else return 0;
	}
	return 0;
}

void filter(list<string> &CoList, vector<region> &RgList, vector<string> &master, char *OutputDir)
// Write .nmp files for each country
{	for (list<string>::iterator c = CoList.begin(); c != CoList.end(); c++)
	{	vector<string> output;
		for (unsigned int l = 1; l < master.size(); l+=2)
		{	if ( RgMatch(RgList, *c, master[l-1].substr(0, master[l-1].find('.'))) \
			||   RgMatch(RgList, *c, master[l].substr(0, master[l].find('.'))) )
			{	output.emplace_back(master[l-1]);
				output.emplace_back(master[l]);
			}
		}
		//cout << "output vector created: " << output.size() << " lines.\n";
		if (output.size())
		{	string outfile = OutputDir+*c+".nmp";
			ofstream nmp(outfile.data());
			for (unsigned int l = 0; l < output.size(); l++)
				nmp << output[l] << '\n';
		}
	}
}

int main(int argc, char *argv[])
{	if (argc != 4)
	{	cout << "usage: nmpbycountry RgCsvFile MasterNMP OutputDir\n";
		return 0;
	}

	// Record execution start time
	time_t StartTime = time(0);
	char* LocalTime = ctime(&StartTime);

	// Attempt to find most recent commit info
	string MasterInfo;
	string MasterPath = argv[1];
	MasterPath.erase(MasterPath.find_last_of("/\\")+1);
	MasterPath += ".git/refs/heads/master";
	ifstream MasterFile(MasterPath.data());
	if (MasterFile)
		MasterFile >> MasterInfo;
	else	MasterInfo = "unknown.";

	// nmpbyregion.log
	string LogPath = argv[3];
	LogPath += "nmpbycountry.log";
	ofstream LogFile(LogPath.data());
	LogFile << "nmpbycountry executed " << LocalTime;
	LogFile << "Most recent commit is " << MasterInfo << '\n';

	// The actual filtering
	list<string> CoList;
	vector<region> RgList;
	vector<string> master;
	if (!GetRegions(argv[1], CoList, RgList)) return 0;
	if (!ReadMaster(argv[2], master))	  return 0;
	filter(CoList, RgList, master, argv[3]);
}
