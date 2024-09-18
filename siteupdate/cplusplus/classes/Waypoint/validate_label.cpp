auto validate_label = [&](char* lbl, char* end)
{	char* c = lbl;
	if ([&]() // verify valid sequence of leading + or *
	{	if (*c == '+' || *c == '*')
		  if (*++c == '+') return 1;
		  else if (*c == '*')
		  {	if (c++[-1] == '*') return 1;
		  }
		  else if (!*c) return 1;
		return 0;
	}())	return Datacheck::add(rte, lbl, "", "", "LABEL_INVALID_CHAR", "");

	bool flag = 0;	// once set to 1, keep going to find bad chars that can't go in DB, logfile, or terminal
	do {	if	    (*c <= '@')	// 1/2
		  if	    (*c <= ')') // 1/4
		    if	    (*c <  ' '){el.add_error(fmt::format("control character(s) in label at offset {} ({:#X}) in {}/{}/{}.wpt",
						     c-wptdata, c-wptdata, rte->region->code, rte->system->systemname, rte->root));
					// As ErrorList error means no DB is created in order to populate the datacheck page, and putting
					// control chars in the middle of datacheck.log is bad juju, let's skip the datacheck entry and...
					break;}		//| up to 31 | control
		    else if (*c <='\''){if (lbl == line)//|	     |
					   el.add_error(fmt::format("`{}` character in label in {}/{}/{}.wpt",
							*c, rte->region->code, rte->system->systemname, rte->root));
					flag = 1;}	//| 32 to 39 | space !"#$%&'
		    else		continue;	//| 40 to 41 | ()
		  else			// 2/4		//|----------|
		    if	    (*c <= ',')	flag = 1;	//| 42 to 44 | *+,
		    else if (*c <= '9')	continue;	//| 45 to 57 | -./ 0-9
		    else		flag = 1;	//| 58 to 64 | :;<=>?@
		else			// 2/2		//|__________|
		  if	    (*c <= '_')	// 3/4		//|	     |
		    if	    (*c <= 'Z')	continue;	//| 65 to 90 | A-Z
		    else if (*c <= '^'){if (*c == '\\') //|	     |
					   el.add_error(fmt::format("backslash in label in {}/{}/{}.wpt",
							rte->region->code, rte->system->systemname, rte->root));
					flag = 1;}	//| 91 to 94 | [\]^
		    else		continue;	//|    95    | _
		  else			// 4/4		//|__________|
		    if	    (*c <= 'z')	// 7/8		//|	     |
		      if    (*c == '`')	flag = 1;	//|    96    | `
		      else		continue;	//| 97 - 122 | a-z
		    else		// 8/8		//|----------|
		      if    (*c <= '~')	flag = 1;	//| 123..126 | {|}~
		      else	       {el.add_error(fmt::format("ASCII DEL in label at offset {} ({:#X}) in {}/{}/{}.wpt",
						     c-wptdata, c-wptdata, rte->region->code, rte->system->systemname, rte->root));
					// Again, let's skip the datacheck entry and...
					break;}		//|    127   | DEL
	   }	while (++c < end);   /* Yes, I could rearrange the tree to process [\]^ one branch faster.
					But those characters won't be present in clean data anyway, and it
					disrupts the organization of the code & makes things less readable. */
	if (flag)
	{	std::string str = end - lbl <= DBFieldLength::label
		? std::string(lbl, end) // or cut down to fit in DB if needed. Likely to be URL, so save the end
		: std::string("...").append(end - DBFieldLength::label + 3, DBFieldLength::label - 3);
		Datacheck::add(rte, str, "", "", "LABEL_INVALID_CHAR", "");
	}
};
