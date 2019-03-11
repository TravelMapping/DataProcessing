std::string lower(std::string str)
{	for (unsigned int c = 0; c < str.size(); c++)
	  if (str[c] >= 'A' && str[c] <= 'Z') str[c] += 32;
	return str;
}

//#ifdef TravelerList__cstr
char *lower(char *str)
{	for (char *c = str; *c != 0; c++)
	  if (*c >= 'A' && *c <= 'Z') *c += 32;
	return str;
}
//#endif
