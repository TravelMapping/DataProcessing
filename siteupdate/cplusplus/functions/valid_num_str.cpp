#include <cstddef>

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
