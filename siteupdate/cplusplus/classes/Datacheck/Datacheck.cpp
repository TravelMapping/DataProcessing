#include "Datacheck.h"
#include "../ElapsedTime/ElapsedTime.h"
#include "../ErrorList/ErrorList.h"
#include "../Route/Route.h"
#include "../../functions/split.h"
#include <fstream>

std::mutex Datacheck::mtx;
std::list<Datacheck> Datacheck::errors;

void Datacheck::add(Route *rte, std::string l1, std::string l2, std::string l3, std::string c, std::string i)
{	mtx.lock();
	errors.emplace_back(rte, l1, l2, l3, c, i);
	mtx.unlock();
}

Datacheck::Datacheck(Route *rte, std::string l1, std::string l2, std::string l3, std::string c, std::string i)
{	route = rte;
	label1 = l1;
	label2 = l2;
	label3 = l3;
	code = c;
	info = i;
	fp = 0;
}

bool Datacheck::match_except_info(std::string* fpentry)
{	// Check if the fpentry from the csv file matches in all fields
	// except the info field
	if (fpentry[0] != route->root)	return 0;
	if (fpentry[1] != label1)	return 0;
	if (fpentry[2] != label2)	return 0;
	if (fpentry[3] != label3)	return 0;
	if (fpentry[4] != code)		return 0;
	return 1;
}

// Original "Python list" format unused. Using "CSV style" format instead.
std::string Datacheck::str() const
{	return route->root + ";" + label1 + ";" + label2 + ";" + label3 + ";" + code + ";" + info;
}

void Datacheck::read_fps(std::string& path, ErrorList &el)
{	// read in the datacheck false positives list
	std::ifstream file(path+"/datacheckfps.csv");
	std::string line;
	getline(file, line); // ignore header line
	while (getline(file, line))
	{	// trim DOS newlines & trailing whitespace
		while (line.back() == 0x0D || line.back() == ' ' || line.back() == '\t')
			line.pop_back();
		// trim leading whitespace
		while (line[0] == ' ' || line[0] == '\t')
			line = line.substr(1);
		if (line.empty()) continue;
		// parse datacheckfps.csv line
		size_t NumFields = 6;
		std::string* fields = new std::string[6];
				 // deleted when FP is matched or on termination of program
		std::string* ptr_array[6] = {&fields[0], &fields[1], &fields[2], &fields[3], &fields[4], &fields[5]};
		split(line, ptr_array, NumFields, ';');
		if (NumFields != 6)
		{	el.add_error("Could not parse datacheckfps.csv line: [" + line
				   + "], expected 6 fields, found " + std::to_string(NumFields));
			continue;
		}
		if (always_error.find(fields[4]) != always_error.end())
			std::cout << "datacheckfps.csv line not allowed (always error): " << line << std::endl;
		else	fps.push_back(fields);
	}
	file.close();
}

void Datacheck::mark_fps(std::string& path, ElapsedTime &et)
{	errors.sort();
	std::ofstream fpfile(path+"/nearmatchfps.log");
	time_t timestamp = time(0);
	fpfile << "Log file created at: " << ctime(&timestamp);
	unsigned int counter = 0;
	unsigned int fpcount = 0;
	for (Datacheck& d : errors)
	{	//std::cout << "Checking: " << d->str() << std::endl;
		counter++;
		if (counter % 1000 == 0) std::cout << '.' << std::flush;
		for (std::list<std::string*>::iterator fp = fps.begin(); fp != fps.end(); fp++)
		  if (d.match_except_info(*fp))
		    if (d.info == (*fp)[5])
		    {	//std::cout << "Match!" << std::endl;
			d.fp = 1;
			fpcount++;
			fps.erase(fp);
			delete[] *fp;
			break;
		    }
		    else
		    {	fpfile << "FP_ENTRY: " << (*fp)[0] << ';' << (*fp)[1] << ';' << (*fp)[2] << ';' << (*fp)[3] << ';' << (*fp)[4] << ';' << (*fp)[5] << '\n';
			fpfile << "CHANGETO: " << (*fp)[0] << ';' << (*fp)[1] << ';' << (*fp)[2] << ';' << (*fp)[3] << ';' << (*fp)[4] << ';' << d.info << '\n';
		    }
	}
	fpfile.close();
	std::cout << '!' << std::endl;
	std::cout << et.et() << "Found " << Datacheck::errors.size() << " datacheck errors and matched " << fpcount << " FP entries." << std::endl;
}

void Datacheck::unmatchedfps_log(std::string& path)
{	// write log of unmatched false positives from the datacheckfps.csv
	std::ofstream fpfile(path+"/unmatchedfps.log");
	time_t timestamp = time(0);
	fpfile << "Log file created at: " << ctime(&timestamp);
	if (fps.empty()) fpfile << "No unmatched FP entries.\n";
	else for (std::string* entry : fps)
		fpfile << entry[0] << ';' << entry[1] << ';' << entry[2] << ';' << entry[3] << ';' << entry[4] << ';' << entry[5] << '\n';
	fpfile.close();
}

void Datacheck::datacheck_log(std::string& path)
{	std::ofstream logfile(path+"/datacheck.log");
	time_t timestamp = time(0);
	logfile << "Log file created at: " << ctime(&timestamp);
	logfile << "Datacheck errors that have been flagged as false positives are not included.\n";
	logfile << "These entries should be in a format ready to paste into datacheckfps.csv.\n";
	logfile << "Root;Waypoint1;Waypoint2;Waypoint3;Error;Info\n";
	if (errors.empty()) logfile << "No datacheck errors found.\n";
	else for (Datacheck& d : errors)
		if (!d.fp) logfile << d.str() << '\n';
	logfile.close();
}

bool operator < (const Datacheck &a, const Datacheck &b)
{	return a.str() < b.str();
}

std::list<std::string*> Datacheck::fps;

std::unordered_set<std::string> Datacheck::always_error
{	"ABBREV_AS_CHOP_BANNER",
	"ABBREV_AS_CON_BANNER",
	"BAD_ANGLE",
	"CON_BANNER_MISMATCH",
	"CON_ROUTE_MISMATCH",
	"DISCONNECTED_ROUTE",
	"DUPLICATE_LABEL",
	"HIDDEN_TERMINUS",
	"INTERSTATE_NO_HYPHEN",
	"INVALID_FINAL_CHAR",
	"INVALID_FIRST_CHAR",
	"LABEL_INVALID_CHAR",
	"LABEL_PARENS",
	"LABEL_SLASHES",
	"LABEL_TOO_LONG",
	"LABEL_UNDERSCORES",
	"LONG_UNDERSCORE",
	"MALFORMED_LAT",
	"MALFORMED_LON",
	"MALFORMED_URL",
	"NONTERMINAL_UNDERSCORE",
	"US_LETTER"
};
