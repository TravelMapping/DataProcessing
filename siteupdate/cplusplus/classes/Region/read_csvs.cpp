#include "Region.h"
#include "../Args/Args.h"
#include "../DBFieldLength/DBFieldLength.h"
#include "../ErrorList/ErrorList.h"
#include "../../functions/tmstring.h"
#include <fstream>

void Region::cccsv(ErrorList& el, std::string filename, std::string unit, size_t codelen, size_t namelen, std::vector<std::pair<std::string, std::string>>& container)
{	std::ifstream file;
	std::string line;
	file.open(Args::datapath+'/'+filename);
	if (!file) el.add_error("Could not open "+Args::datapath+'/'+filename);
	else {	getline(file, line); // ignore header line
		while(getline(file, line))
		{	if (line.size() && line.back() == 0x0D) line.pop_back();	// trim DOS newlines
			if (line.empty()) continue;
			size_t delim = line.find(';');
			if (delim == -1)
			{	el.add_error("Could not parse "+filename+" line: ["+line+"], expected 2 fields, found 1");
				continue;
			}
			std::string code(line,0,delim);
			std::string name(line,delim+1);
			if (name.find(';') != -1)
			{	el.add_error("Could not parse "+filename+" line: ["+line+"], expected 2 fields, found 3");
				continue;
			}
			// verify field lengths
			if (code.size() > codelen) el.add_error(unit+" code > "+std::to_string(codelen)+" bytes in "+filename+" line "+line);
			if (name.size() > namelen) el.add_error(unit+" name > "+std::to_string(namelen)+" bytes in "+filename+" line "+line);
			container.emplace_back(code, name);
		}
	     }
	file.close();
	// create a dummy item to catch unrecognized codes in .csv files
	container.emplace_back("error", "unrecognized "+unit+" code");
}

void Region::read_csvs(ErrorList& el)
{	cccsv(el, "continents.csv", "Continent", DBFieldLength::continentCode, DBFieldLength::continentName, continents);
	cccsv(el, "countries.csv",  "Country",   DBFieldLength::countryCode,   DBFieldLength::countryName,   countries);

	// regions.csv
	std::ifstream file;
	std::string line;
	file.open(Args::datapath+"/regions.csv");
	if (!file)
	     {	el.add_error("Could not open "+Args::datapath+"/regions.csv");
		it = allregions.alloc(1);
	     }
	else {	getline(file, line); // ignore header line
		std::list<std::string> lines;
		while(getline(file, line))
		{	if (line.size() && line.back() == 0x0D) line.pop_back();	// trim DOS newlines
			if (line.size()) lines.emplace_back(move(line));
		}
		lines.sort(sort_1st_csv_field);
		it = allregions.alloc(lines.size()+1);
		for (std::string& l : lines)
		  try {	new(it) Region(l, el);
			// placement new
			code_hash[it->code] = it;
			it++;
		      }
		  catch (const int) {--allregions.size;}
	     }
	file.close();
	// create a dummy region to catch unrecognized region codes in .csv files
	new(it) Region("error;unrecognized region code;error;error;unrecognized region code", el);
	// placement new
	code_hash[it->code] = it;
}
