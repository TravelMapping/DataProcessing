#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <list>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
using namespace std;

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
		delete[] l;
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
		char* l = new char[ConstLine.size()+2];
		strcpy(l, ConstLine.data());
		l[ConstLine.size()+1] = 0;
		BegI = strtoul(l, &c, 10);	BegP = v_vector[BegI];	c++;
		EndI = strtoul(c, &c, 10);	EndP = v_vector[EndI];	c++;
		label = strtok(c, " ");		c += label.size();
			// strtok(0, FOO) is not threadsafe. Don't use.

		if (traveled)
		{	clinchedby_code = strtok(c+1, " ");
			c += clinchedby_code.size() + 1;
		}

		while (*(c+1))
		{	iLat.push_back(strtod(c+1, &c));
			iLon.push_back(strtod(c+1, &c));
		}

		delete[] l;

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

class tmg
{	public:
	string pathname, destination, tag, version, format;
	size_t NumVertices, NumEdges, NumTrav;
	bool opened, traveled;

	tmg(string&& entry, std::string&& dest)
	{	pathname = entry;
		destination = dest;
		opened = 1;
		NumVertices = 0;
		NumEdges = 0;
		NumTrav = -1;
		ifstream file(pathname);
		if (!file.is_open())
		{	opened = 0;
			return;
		}
		file >> tag >> version >> format;
		traveled = format == "traveled";
		// enforce newline between header lines
		string line;
		getline(file, line);
		file >> NumVertices >> NumEdges;
		if (traveled) file >> NumTrav;
		file.close();
	}

	bool valid_header()
	{	if (!opened)
		{	cout << "Unable to open file. ";
			return 0;
		}
		bool valid = 1;
		if (tag != "TMG") valid = 0;
		else	if (version != "1.0")
		{	if (version != "2.0" || !traveled) valid = 0;
		}
		else	if (format != "collapsed" && format != "simple") valid = 0;
		if (!valid)
		{	cout << tag << ' ' << version << ' ' << format << " unsupported. ";
			return 0;
		}
		if (!NumVertices)
		{	cout << "No vertices. ";
			valid = 0;
		}
		if (!NumEdges)
		{	cout << "No edges. ";
			valid = 0;
		}
		if (traveled && NumTrav == -1)
		{	cout << "No valid traveler count. ";
			valid = 0;
		}
		return valid;
	}

	std::string filename()
	{	return pathname.substr(pathname.find_last_of('/')+1);
	}

	void canonicaltmg();
};

bool SortGraphs(tmg& g1, tmg& g2)
{	return g1.NumEdges < g2.NumEdges;
}

void tmg::canonicaltmg()
{	ifstream input(pathname);
	string tmgline;
	vector<vertex*> v_vector;
	list<edge*> e_list;

	// skip header; we've already read it
	getline(input, tmgline);
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
	input.close();

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
	ofstream output(destination);
	output << tag << ' ' << version << ' ' << format << '\n';
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
	if (traveled) output << tmgline << '\n';

	// delete vertices & edges
	for (vertex* v : v_vector) delete v;
	for (  edge* e : e_list  ) delete e;
}

void GraphThread(size_t id, std::vector<tmg>* graphvec, size_t* index, std::mutex* mtx, std::string* msg)
{	//std::cout << "Starting GraphThread " << id << std::endl;
	while (*index < graphvec->size())
	{	mtx->lock();
		if (*index >= graphvec->size())
		{	mtx->unlock();
			return;
		}
		//std::cout << "Thread " << id << " with graphvec.size()=" << graphvec->size() << " & index=" << *index << std::endl;
		//std::cout << "Thread " << id << " assigned " << graphvec->at(*index).filename() << std::endl;
		size_t i = *index;
		(*index)++;
		cout << string(msg->size(), ' ') << '\r';
		*msg = '[' + to_string(*index) + '/' + to_string(graphvec->size()) + "] " + graphvec->at(i).filename();
		cout << *msg << flush << '\r';
		mtx->unlock();
		graphvec->at(i).canonicaltmg();
	}
}

int main(int argc, char* argv[])
{	list<tmg> graphlist;
	char *src, *dest;
	size_t i = 0;
	size_t T = 0;
	size_t t;
	string msg;
	mutex mtx;

	switch (argc)
	{	case 3 : src = argv[1];
			 dest = argv[2];
			 break;
		case 5 : if (!strcmp(argv[1], "-t"))
			 {	T = strtoul(argv[2], 0, 10);
				src = argv[3];
				dest = argv[4];
				break;
			 }
		default: cout << "usage: " << argv[0] <<" [-t NumThreads] <InputPath> <OutputPath>\n"; return 1;
	}
	if (!T) T = thread::hardware_concurrency();
	if (!T) T = 1;
	vector<thread> thr(T);

	DIR *dir;
	dirent *ent;
	if ((dir = opendir(src)) != NULL)
	     {	while ((ent = readdir(dir)) != NULL)
		{	string entry = string(src) + "/" + ent->d_name;
			if (entry.substr(entry.size()-4) == ".tmg")
			{	graphlist.emplace_back( move(entry), dest + entry.substr(entry.find_last_of('/')) );
				if (!graphlist.back().valid_header())
				{	cout << "Skipping " << graphlist.back().pathname << endl;
					graphlist.pop_back();
				}
			}
		}
		closedir(dir);
	     }
	else {	graphlist.emplace_back(src, dest);
		if (!graphlist.back().valid_header())
		{	cout << "Skipping " << graphlist.back().pathname << endl;
			graphlist.pop_back();
		}
	     }

	graphlist.sort(SortGraphs);
	vector<tmg> graphvec(graphlist.rbegin(), graphlist.rend());
	for (t=0;t<T;t++) thr[t] = thread(GraphThread, t, &graphvec, &i, &mtx, &msg);
	for (t=0;t<T;t++) thr[t].join();
	cout << endl;
}
