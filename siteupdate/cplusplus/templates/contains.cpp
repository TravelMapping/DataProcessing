template <class container, class item>
inline bool contains(const container &haystack, const item &needle)
{	for (const item &i : haystack)
	  if (i == needle)
	    return 1;
	return 0;
}
