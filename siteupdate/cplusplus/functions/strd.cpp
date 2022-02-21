const char* strdstr(const char* h, const char* n, const char& d)
{	for (; *h && *h != d; h++)
	{	const char* a = h;
		const char* b = n;
		while (*a != d)
		  if (*a++ != *b++) break;
		  else if (!*b) return h;
	}
	return 0;
}

int strdcmp(const char* a, const char* b, const char& d)
{	do	if (!*b) return *a == d ? 0 : *a-*b;
	while	(*a++ == *b++);
	return	*--a - *--b;
}
