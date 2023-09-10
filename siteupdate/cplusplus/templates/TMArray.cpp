#ifndef TMARRAY
#define TMARRAY

#include <cstdlib>

template <class item>
struct TMArray
{	item * data;
	size_t size;

	TMArray(): data(0), size(0) {}
	~TMArray()
	{	for (auto& i : *this) i.~item();
		free(data);
	}

	item* alloc(size_t s)
	{	size=s;
		return data = (item*)aligned_alloc(alignof(item), s*sizeof(item));
	}

	item& operator[](size_t i)
			const {return data[i];}
	item* begin()	const {return data;}
	item* end()	const {return data+size;}
	item& back()	const {return data[size-1];}
};
#endif
