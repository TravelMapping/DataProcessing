// Travel Mapping Project, Eric Bryant, 2017
using namespace std;
#include <fstream>
#include <iostream>

class entry
{	public:
	string text;
	bool cull;
	entry *prev, *next;

	// init alpha
	entry() {prev = 0; next = 0; cull = 0;}
	// insertions
	entry(entry *cursor) // cursor, in this case, is a pointer to the prev entry
	{	prev = cursor;
		next = 0;
		cull = 0;
	}
};

bool find(string needle, entry *haystack)
{	while (haystack)
	{	if (needle == haystack->text) return 1;
		haystack = haystack->next;
	}
	return 0;
}

int main(int argc, char *argv[])
{	entry *cList = new entry;
	entry *fList = new entry;
	entry *cursor;
	if (argc != 4) { cout << "usage: ./fpcull CullFile FullFile OutputFile\n"; return 0; }
	// init ifstreams
	ifstream cFile(argv[1], ios::in);
	if (!cFile.is_open())	{ cout << argv[1] << " file not found!" << endl; return 0; }
	ifstream fFile(argv[2], ios::in);
	if (!fFile.is_open())	{ cout << argv[2] << " file not found!" << endl; return 0; }

	// create cull list
	cFile >> cList->text; //TODO: enclose in an IF in case hell breaks loose (IE, no text in cFile) //FIXME: see below
	cursor = new entry(cList);
	while (cFile >> cursor->text) //FIXME: This expects every line to be one text string and every text string to be one line. This will not always be the case.
	// For example, see line 6613 of https://github.com/TravelMapping/HighwayData/blob/d33d0f6e93eec8ce7f71f31af94336a473161e5f/datacheckfps.csv
	{	cursor->prev->next = cursor;
		cursor = new entry(cursor);
	}
	delete cursor;

	// create full list
	fFile >> fList->text; //TODO: enclose in an IF in case hell breaks loose (IE, no text in fFile) //FIXME: see below
	cursor = new entry(fList);
	while (fFile >> cursor->text) //FIXME: This expects every line to be one text string and every text string to be one line. This will not always be the case.
	// For example, see line 6613 of https://github.com/TravelMapping/HighwayData/blob/d33d0f6e93eec8ce7f71f31af94336a473161e5f/datacheckfps.csv
	{	cursor->prev->next = cursor;
		cursor = new entry(cursor);
	}
	delete cursor;

	// flag unmatchedfps as to-be-culled
	for (cursor = fList; cursor; cursor = cursor->next)
		if (find(cursor->text, cList)) cursor->cull = 1;

	// output
	ofstream oFile(argv[3]);
	cursor = fList;
	while (cursor)
	{	if (!cursor->cull) oFile << cursor->text << endl;
		cursor = cursor->next;
	}
}
