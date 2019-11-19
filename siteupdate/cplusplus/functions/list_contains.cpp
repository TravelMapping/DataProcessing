template <class item>
inline bool list_contains(const std::list<item> *haystack, const item &needle)
{	for (const item &i : *haystack)
	  if (i == needle)
	    return 1;
	return 0;
}
