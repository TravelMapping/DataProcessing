// Travel Mapping Project, Eric Bryant, 2018
#include <fstream>
#include <iostream>
#include <string>
using namespace std;

int main(int argc, char *argv[])
{	if (argc != 3)
	{	cout << "usage: nmplifilter InputFile OutputFile\n";
		return 0;
	}
	ifstream in(argv[1]);
	if (!in)
	{	std::cout << argv[1] << " not found\n";
		return 0;
	}
	string pt1, pt2;
	ofstream out(argv[2]);
	while (getline(in, pt1) && getline(in, pt2))
	{	if (pt1.substr(pt1.size()-2, 2) == "LI" ||  pt2.substr(pt2.size()-2, 2) == "LI")
		out << pt1 << '\n' << pt2 << '\n';
	}
}
