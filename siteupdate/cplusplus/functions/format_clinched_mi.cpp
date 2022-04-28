#include "format_clinched_mi.h"
#include <cstdio>

char* format_clinched_mi(char* str, double clinched, double total)
{	/* return a nicely-formatted string for a given number of miles
	clinched and total miles, including percentage */
	if (total)
		sprintf(str, "%0.2f of %0.2f mi (%0.2f%%)", clinched, total, 100*clinched/total);
	else	sprintf(str, "%0.2f of %0.2f mi -.--%%", clinched, total);
	return str;
}
