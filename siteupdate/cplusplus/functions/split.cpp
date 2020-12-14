#include "split.h"

void split(const std::string& line, std::string* f[], size_t &s, const char delim)
{	size_t i = 0;
	size_t r = 0;
	for (size_t l = 0; r != -1 && i < s; l = r+1)
	{	r = line.find(delim, l);
		f[i]->assign(line, l, r-l);
		i++;
	}
	s = (r == -1) ? i : s+1;
}
