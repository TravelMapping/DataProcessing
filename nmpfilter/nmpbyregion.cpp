// Travel Mapping Project, Eric Bryant, 2018
#include <ctime>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

bool LwrCaseValid(string &RgStr)
// Check whether a string is a valid region; convert to lower case
{	if (RgStr == ".")		return 0;
	if (RgStr == "..")		return 0;
	if (RgStr == "_systems")	return 0;
	if (RgStr == "_boundaries")	return 0;
	for (unsigned int i = 0; i < RgStr.size(); i++)
	  if (RgStr[i] >= 'A' && RgStr[i] <= 'Z')
	    RgStr[i] += 32;
	return 1;
}

void GetRegions(char *HwyDataDir, vector<string> &RgList)
// Get a list of regions by scanning subdirectories of HwyData/
{	DIR *dir;
	dirent *ent;
	if ((dir = opendir (HwyDataDir)) != NULL)
	{	while ((ent = readdir (dir)) != NULL)
		{	string RgStr = ent->d_name;
			if (LwrCaseValid(RgStr)) RgList.push_back(RgStr);
		}
		closedir(dir);
	}
	else cout << "Error opening directory " << HwyDataDir << ". (Not found?)\n";
}

string GetRootRg(string RgStr)
// Remove dashes from region codes, to match region strings as found in Roots
{	for (int dash = RgStr.find('-'); dash > 0; dash = RgStr.find('-'))
		RgStr.erase(dash, 1);
	return RgStr;
}

void filter(vector<string> &RgList, char *MasterNMP, char *OutputDir)
// Write .nmp files for each region
{	for (unsigned int i = 0; i < RgList.size(); i++)
	{	ifstream master(MasterNMP);
		if (!master)
		{	std::cout << MasterNMP << " not found\n";
			return;
		}
		string pt1, pt2;
		string outfile = OutputDir+RgList[i]+".nmp";
		ofstream nmp(outfile.data());
		while (getline(master, pt1) && getline(master, pt2))
		{	string RootRg = GetRootRg(RgList[i]);
			if (pt1.substr(0, pt1.find('.')) == RootRg || pt2.substr(0, pt2.find('.')) == RootRg)
			nmp << pt1 << '\n' << pt2 << '\n';
		}
	}
}

int main(int argc, char *argv[])
{	if (argc != 4)
	{	cout << "usage: nmpbyregion HwyDataDir MasterNMP OutputDir\n";
		return 0;
	}

	// Record execution start time
	time_t StartTime = time(0);
	char* LocalTime = ctime(&StartTime);

	// Attempt to find most recent commit info
	string MasterInfo;
	string MasterPath = argv[1];
	MasterPath += "../.git/refs/heads/master";
	ifstream MasterFile(MasterPath.data());
	if (MasterFile)
		MasterFile >> MasterInfo;
	else	MasterInfo = "unknown.";

	// nmpbyregion.log
	string LogPath = argv[3];
	LogPath += "nmpbyregion.log";
	ofstream LogFile(LogPath.data());
	LogFile << "nmpbyregion executed " << LocalTime;
	LogFile << "Most recent commit is " << MasterInfo << '\n';

	// The actual filtering
	vector<string> RgList;
	GetRegions(argv[1], RgList);
	filter(RgList, argv[2], argv[3]);
}
