#include "double_quotes.h"

std::string double_quotes(std::string str)
{	for (size_t i = 0; i < str.size(); i++)
	  if (str[i] == '\'')
	  {	str.replace(i, 1, "''");
		i++;
	  }
	return str;
}
