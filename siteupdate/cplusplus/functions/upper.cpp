std::string upper(std::string str)
{	for (unsigned int c = 0; c < str.size(); c++)
	  if (str[c] >= 'a' && str[c] <= 'z') str[c] -= 32;
	return str;
}
