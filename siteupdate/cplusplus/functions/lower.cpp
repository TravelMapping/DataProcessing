#include "lower.h"

const char *lower(const char *str)
{	for (char* c = (char*)str; *c != 0; c++)
	  if (*c >= 'A' && *c <= 'Z') *c += 32;
	return str;
}
