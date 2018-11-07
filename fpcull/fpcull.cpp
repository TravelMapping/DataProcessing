// Travel Mapping Project, Eric Bryant, 2017, 2018
#include <fstream>
#include <iostream>
#include <list>
using namespace std;

int main(int argc, char *argv[])
{	string FPline;
	if (argc != 4) { cout << "usage: ./fpcull CullFile FullFile OutputFile\n"; return 0; }
	// init ifstreams
	ifstream cFile(argv[1], ios::in);
	if (!cFile.is_open())	{ cout << argv[1] << " file not found!\n"; return 0; }
	ifstream fFile(argv[2], ios::in);
	if (!fFile.is_open())	{ cout << argv[2] << " file not found!\n"; return 0; }

	// create full list
	list<string> fList;
	while (getline(fFile, FPline)) fList.push_back(FPline);

	// read cull list...
	while (getline(cFile, FPline))
	{	list<string>::iterator cursor = fList.begin();
		// advance thru full FP list until current unmatched entry found...
		while (*cursor != FPline && cursor != fList.end()) cursor++;
		// ...and delete it.
		if (cursor != fList.end()) cursor = fList.erase(cursor);
	}

	// output
	ofstream oFile(argv[3]);
	for (list<string>::iterator cursor = fList.begin(); cursor != fList.end(); cursor++)
		oFile << *cursor << '\n';
}
