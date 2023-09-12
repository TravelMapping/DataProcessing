#include "tmstring.h"

bool sort_1st_csv_field(const std::string& a, const std::string& b)
{	return strdcmp(a.data(), b.data(), ';') < 0;
}

void split(const std::string& line, std::string** f, size_t& s, const char delim)
{	size_t i = 0;
	size_t r = 0;
	for (size_t l = 0; r != -1 && i < s; l = r+1)
	{	r = line.find(delim, l);
		f[i]->assign(line, l, r-l);
		i++;
	}
	s = (r == -1) ? i : s+1;
}

const char* lower(const char* str)
{	for (char* c = (char*)str; *c != 0; c++)
	  if (*c >= 'A' && *c <= 'Z') *c += 32;
	return str;
}

const char* upper(const char* str)
{	for (char* c = (char*)str; *c; c++)
	  if (*c >= 'a' && *c <= 'z') *c -= 32;
	return str;
}

bool valid_num_str(const char* c, const char delim)
{	size_t point_count = 0;
	// check initial character
	if (*c == '.') point_count = 1;
	else if (*c < '0' && *c != '-' || *c > '9') return 0;
	// check subsequent characters
	for (c++; *c && *c != delim; c++)
	{	// check for minus sign not at beginning
		if (*c == '-') return 0;
		// check for multiple decimal points
		if (*c == '.')
		{	point_count += 1;
			if (point_count > 1) return 0;
		}
		// check for invalid characters
		else if (*c < '0' || *c > '9') return 0;
	}
	return 1;
}

int strdcmp(const char* a, const char* b, const char d)
{	do	if	(!*b || *b==d) return (!*a || *a==d) ? 0 : *a;
		else if (!*a || *a==d) return -*b;
	while	(*a++ == *b++);
	return	*--a - *--b;
}

const char* strdstr(const char* h, const char* n, const char d)
{	for (; *h && *h != d; h++)
	{	const char* a = h;
		const char* b = n;
		while (*a != d)
		  if (*a++ != *b++) break;
		  else if (!*b) return h;
	}
	return 0;
}

char* format_clinched_mi(char* str, double clinched, double total)
{	/* return a nicely-formatted string for a given number of miles
	clinched and total miles, including percentage */
	if (total)
		sprintf(str, "%0.2f of %0.2f mi (%0.2f%%)", clinched, total, 100*clinched/total);
	else	sprintf(str, "%0.2f of %0.2f mi -.--%%", clinched, total);
	return str;
}

std::string double_quotes(std::string str)
{	for (size_t i = 0; i < str.size(); i++)
	  if (str[i] == '\'')
	  {	str.replace(i, 1, "''");
		i++;
	  }
	return str;
}
