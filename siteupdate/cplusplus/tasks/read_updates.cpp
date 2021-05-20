// Read updates.csv file, just keep in the fields list for now since we're
// just going to drop this into the DB later anyway
list<string*> updates;
cout << et.et() << "Reading updates file." << endl;
file.open(Args::highwaydatapath+"/updates.csv");
getline(file, line); // ignore header line
while (getline(file, line))
{	// trim DOS newlines & trailing whitespace
	while (line.back() == 0x0D || line.back() == ' ' || line.back() == '\t')
		line.pop_back();
	if (line.empty()) continue;
	// parse updates.csv line
	size_t NumFields = 5;
	string* fields = new string[5];
			 // deleted on termination of program
	string* ptr_array[5] = {&fields[0], &fields[1], &fields[2], &fields[3], &fields[4]};
	split(line, ptr_array, NumFields, ';');
	if (NumFields != 5)
	{	el.add_error("Could not parse updates.csv line: [" + line
			   + "], expected 5 fields, found " + std::to_string(NumFields));
		continue;
	}
	// date
	if (fields[0].size() > DBFieldLength::date)
		el.add_error("date > " + std::to_string(DBFieldLength::date)
			   + " bytes in updates.csv line " + line);
	// region
	if (fields[1].size() > DBFieldLength::countryRegion)
		el.add_error("region > " + std::to_string(DBFieldLength::countryRegion)
			   + " bytes in updates.csv line " + line);
	// route
	if (fields[2].size() > DBFieldLength::routeLongName)
		el.add_error("route > " + std::to_string(DBFieldLength::routeLongName)
			   + " bytes in updates.csv line " + line);
	// root
	if (fields[3].size() > DBFieldLength::root)
		el.add_error("root > " + std::to_string(DBFieldLength::root)
			   + " bytes in updates.csv line " + line);
	// description
	if (fields[4].size() > DBFieldLength::updateText)
		el.add_error("description > " + std::to_string(DBFieldLength::updateText)
			   + " bytes in updates.csv line " + line);

	// error checks
	if (	fields[0].size() != 10
	     || !isdigit(fields[0][0]) || !isdigit(fields[0][1])
	     || !isdigit(fields[0][2]) || !isdigit(fields[0][3])
	     || fields[0][4] != '-'
	     || fields[0][5] < '0' || fields[0][5] > '1' || !isdigit(fields[0][6])
	     || fields[0][7] != '-'
	     || fields[0][8] < '0' || fields[0][8] > '3' || !isdigit(fields[0][9])
	   )	el.add_error("no valid YYYY-MM-DD date in updates.csv line " + line);
	if (fields[2].empty()) el.add_error("no route name in updates.csv line " + line);

	updates.push_back(fields);
	try   {	Route* r = Route::root_hash.at(fields[3]);
		if (r->last_update == 0 || r->last_update[0] < fields[0])
			r->last_update = updates.back();
	      }
	catch (const std::out_of_range& oor) {}
}
file.close();

// Same plan for systemupdates.csv file, again just keep in the fields
// list for now since we're just going to drop this into the DB later
// anyway
list<string*> systemupdates;
cout << et.et() << "Reading systemupdates file." << endl;
file.open(Args::highwaydatapath+"/systemupdates.csv");
getline(file, line);  // ignore header line
while (getline(file, line))
{	if (line.back() == 0x0D) line.erase(line.end()-1);	// trim DOS newlines
	if (line.empty()) continue;
	// parse systemupdates.csv line
	size_t NumFields = 5;
	string* fields = new string[5];
			 // deleted on termination of program
	string* ptr_array[5] = {&fields[0], &fields[1], &fields[2], &fields[3], &fields[4]};
	split(line, ptr_array, NumFields, ';');
	if (NumFields != 5)
	{	el.add_error("Could not parse systemupdates.csv line: [" + line
			   + "], expected 5 fields, found " + std::to_string(NumFields));
		continue;
	}
	// date
	if (fields[0].size() > DBFieldLength::date)
		el.add_error("date > " + std::to_string(DBFieldLength::date)
			   + " bytes in systemupdates.csv line " + line);
	// region
	if (fields[1].size() > DBFieldLength::countryRegion)
		el.add_error("region > " + std::to_string(DBFieldLength::countryRegion)
			   + " bytes in systemupdates.csv line " + line);
	// systemName
	if (fields[2].size() > DBFieldLength::systemName)
		el.add_error("systemName > " + std::to_string(DBFieldLength::systemName)
			   + " bytes in systemupdates.csv line " + line);
	// description
	if (fields[3].size() > DBFieldLength::systemFullName)
		el.add_error("description > " + std::to_string(DBFieldLength::systemFullName)
			   + " bytes in systemupdates.csv line " + line);
	// statusChange
	if (fields[4].size() > DBFieldLength::statusChange)
		el.add_error("statusChange > " + std::to_string(DBFieldLength::statusChange)
			   + " bytes in systemupdates.csv line " + line);
	systemupdates.push_back(fields);
}
file.close();
