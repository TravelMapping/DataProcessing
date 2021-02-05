#include <cstring>
#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include <vector>
using namespace std;

void split(const std::string& line, std::string* f, size_t &s, const char delim)
{	size_t i = 0;
	size_t r = 0;
	for (size_t l = 0; r != -1 && i < s; l = r+1)
	{	r = line.find(delim, l);
		f[i].assign(line, l, r-l);
		i++;
	}
	s = (r == -1) ? i : s+1;
}

class vertex
{	public:
	string label;
	double lat, lon;
	size_t vertex_num;

	vertex(string &ConstLine)
	{	char* c;
		char* l = new char[ConstLine.size()+1];
		strcpy(l, ConstLine.data());
		c = strchr(l, ' ')+1; *(c-1) = 0; label = l;
		lat = strtod(c, &c); c++;
		lon = strtod(c, 0);
		vertex_num = -1;
	}	
};

bool SortVertices(vertex* v1, vertex* v2)
{	//return v1->label < v2->label;
	if (v1->lat < v2->lat)	return 1;
	if (v1->lat > v2->lat)	return 0;
	return v1->lon < v2->lon;
}

class edge
{	public:
	size_t BegI, EndI;
	//size_t qty;
	vertex* BegP,* EndP;
	string label, clinchedby_code;
	vector<double> iLat, iLon;

	edge(string &ConstLine, vector<vertex*> &v_vector, bool traveled)
	{	char* c;
		char* l = new char[ConstLine.size()+1];
		strcpy(l, ConstLine.data());
		BegI = strtoul(l, &c, 10);	BegP = v_vector[BegI];	c++;
		EndI = strtoul(c, &c, 10);	EndP = v_vector[EndI];	c++;
		label = strtok(c, " ");		c += label.size();
			// strtok(0, FOO) is not threadsafe. Don't use.

		if (traveled)
		{	clinchedby_code = strtok(c+1, " ");
			c += clinchedby_code.size() + 1;
		}

		while (*c)
		{	iLat.push_back(strtoul(c+1, &c, 10));
			iLon.push_back(strtoul(c+1, &c, 10));
		}

		//qty = 1;
		//for (const char* comma = strchr(label.data(), ','); comma; qty++) comma = strchr(comma+1, ',');
	}
};

bool SortEdges(edge* e1, edge* e2)
{	if (e1->BegP->vertex_num < e2->BegP->vertex_num) return 1;
	if (e1->BegP->vertex_num > e2->BegP->vertex_num) return 0;
	if (e1->EndP->vertex_num < e2->EndP->vertex_num) return 1;
	if (e1->EndP->vertex_num > e2->EndP->vertex_num) return 0;
	if (e1->label < e2->label)			 return 1;
	if (e1->label > e2->label)			 return 0;
	if (e1->iLat.size() < e2->iLat.size())		 return 1;
	if (e1->iLat.size() > e2->iLat.size())		 return 0;
	for (size_t i = 0; i < e1->iLat.size(); i++)
	{	if (e1->iLat[i] < e2->iLat[i])		 return 1;
		if (e1->iLat[i] > e2->iLat[i])		 return 0;
		if (e1->iLon[i] < e2->iLon[i])		 return 1;
		if (e1->iLon[i] > e2->iLon[i])		 return 0;
	} // we should never exit this loop
	return 0;
}

void reverse(vector<double>& v)
{	for (size_t i = 0; i < v.size()/2; i++) std::swap(v[i], v[v.size()-i-1]);
}

int main(int argc, char* argv[])
{	ifstream input(argv[1]);
	if (argc < 3)	{ cout << "usage: ./canonicaltmg <InputFile> <OutputFile>\n"; return 0; }
	if (!input)	{ cout << argv[1] << " not found.\n"; return 0; }

	const char* filename = &argv[1][string(argv[1]).find_last_of("/\\")+1];
	cout << filename << "   ->   " << &argv[2][string(argv[2]).find_last_of("/\\")+1] << endl;;

	// read TMG header
	size_t NumVertices, NumEdges, NumTrav;
	string firstline, tmgline;
	getline(input, firstline);

	size_t NumFields = 3;
	vector<string> fields(3);
	split(firstline, fields.data(), NumFields, ' ');
	if ( NumFields != 3 || fields[0] != "TMG" || (fields[2] != "simple" && fields[2] != "collapsed" && fields[2] != "traveled") )
	{	cout << '\"' << firstline << "\" unsupported.\n";
		return 0;
	}
	bool traveled = fields[2] == "traveled";

	input >> NumVertices;
	input >> NumEdges;
	if (traveled) input >> NumTrav;
	vector<vertex*> v_vector;
	list<edge*> e_list;
	// seek to end of 2nd line
	getline(input, tmgline);

	// read vertices
	for (size_t i = 0; i < NumVertices; i++)
	{	getline(input, tmgline);
		v_vector.push_back(new vertex(tmgline));
		//cout << v_vector[i]->label << ' ' << to_string(v_vector[i]->lat) << ' ' << to_string(v_vector[i]->lon) << '\n';
	}

	// read edges
	for (size_t i = 0; i < NumEdges; i++)
	{	getline(input, tmgline);
		e_list.push_back(new edge(tmgline, v_vector, traveled));
		/*cout << edges.back()->BegI << ' ' << edges.back()->EndI << ' ' << edges.back()->label;
		  for (double s : edges.back()->shape) cout << ' ' << s;
		  cout << '\n';//*/
	}

	// read traveler lists
	if (traveled) getline(input, tmgline);

	// sort vertices
	list<vertex*> v_list (v_vector.begin(), v_vector.end());
	v_list.sort(SortVertices);
	v_vector.assign(v_list.begin(), v_list.end());
	// assign vertex_num
	for (size_t v = 0; v < v_vector.size(); v++) v_vector.at(v)->vertex_num = v;

	// deterministic edge lines
	for (edge* e : e_list)
	{	// store edges with lower-numbered vertex @ beginning
		if (e->BegP->vertex_num > e->EndP->vertex_num)
		{	//cout << "DEBUG: reversing" << e->BegP->label << " <-> " << e->EndP->label << std::endl;
			swap(e->BegP, e->EndP);
			reverse(e->iLat);
			reverse(e->iLon);
		}
		// deterministic intermediate point order on lollipops
		// the size() part shouldn't be necessary; just being safe.
		if (e->BegP == e->EndP && e->iLat.size())
		{	//cout << "DEBUG: lollipop" << std::endl;
			if (e->iLat.front() > e->iLat.back()
			|| (e->iLat.front() == e->iLat.back() && e->iLon.front() > e->iLon.back()))
			{	reverse(e->iLat);
			  	reverse(e->iLon);
			}
		}
	}

	// sort edges
	e_list.sort(SortEdges);

	// write output TMG
	ofstream output(argv[2]);
	output << firstline << '\n';
	output << NumVertices << ' ' << NumEdges;
	if (traveled) output << ' ' << NumTrav;
	output << '\n';
	for (vertex* v : v_vector)
		output << v->label << ' ' << to_string(v->lat) << ' ' << to_string(v->lon) << '\n';
	for (edge* e : e_list)
	{	output << e->BegP->vertex_num << ' ';
		output << e->EndP->vertex_num << ' ';
		output << e->label;
		if (traveled) output << ' ' << e->clinchedby_code;
		// intermediate points
		for (size_t i = 0; i < e->iLat.size(); i++)
		{	output << ' ' << to_string(e->iLat[i]);
			output << ' ' << to_string(e->iLon[i]);
		}
		output << '\n';
	}
	if (traveled) output << tmgline;
}
