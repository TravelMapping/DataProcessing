std::string upper(std::string str)
{	for (unsigned int c = 0; c < str.size(); c++)
	  if (str[c] >= 'a' && str[c] <= 'z') str[c] -= 32;
	return str;
}

const char *upper(const char *str)
{	for (char* c = (char*)str; *c != 0; c++)
	  if (*c >= 'a' && *c <= 'z') *c -= 32;
	return str;
}
