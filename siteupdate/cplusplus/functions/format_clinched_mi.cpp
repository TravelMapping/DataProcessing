#include "format_clinched_mi.h"

std::string format_clinched_mi(double clinched, double total)
{	/* return a nicely-formatted string for a given number of miles
	clinched and total miles, including percentage */
	char str[37];
	std::string percentage = "-.--%";
	if (total)
	{	sprintf(str, "(%0.2f%%)", 100*clinched/total);
		percentage = str;
	}
	sprintf(str, "%0.2f of %0.2f mi ", clinched, total);
	return str + percentage;
}
