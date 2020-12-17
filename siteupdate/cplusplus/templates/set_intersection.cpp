// intersections of unordered sets
template <class item>
std::unordered_set<item> operator & (const std::unordered_set<item> &a, const std::unordered_set<item> &b)
{	std::unordered_set<item> s;
	for (const item &i : a)
	  if (b.find(i) != b.end())
	    s.insert(i);
	return s;
}
