#include "upper.h"

const char *upper(const char *str)
{	for (char* c = (char*)str; *c != 0; c++)
	  if (*c >= 'a' && *c <= 'z') *c -= 32;
	return str;
}
