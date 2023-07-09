#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>

template <class item, class unit> struct TMBImpl;

template <class item, class unit> class TMBitset
{	size_t const units;
	size_t const len;
	item   const start;
	unit * const data;

	static constexpr size_t ubits = 8*sizeof(unit);
	friend struct TMBImpl<item,unit>;

	public:
	TMBitset(item const s, const size_t l):
	  units(ceil(double(l+1)/ubits)),
	  len  (l),
	  start(s),
	  data ( (unit*const)calloc(units, sizeof(unit)) )
	{	//create dummy end() item
		data[len/ubits] |= (unit)1 << len%ubits;
	}

	~TMBitset() {free(data);}

	bool add_index(size_t const index)
	{	const unit u = data[index/ubits];
		data[index/ubits] |= (unit)1 << index%ubits;
		return u != data[index/ubits];
	}

	bool operator [] (const size_t index) const {return data[index/ubits] & (unit)1 << index%ubits;}

	// The != and |= operators assume both objects have the
	// same length, and shall only be used in this context.
	bool operator != (const TMBitset<item,unit>& other) const {return memcmp(data, other.data, units*sizeof(unit));}
	void operator |= (const TMBitset<item,unit>& other) {TMBImpl<item,unit>::bitwise_or(*this, other);}

	class iterator
	{	size_t bit;
		unit* u;
		item val;
		unit bits;

		friend iterator TMBitset::begin() const;

		public:
		iterator(const TMBitset& b, size_t const i): bit(i%ubits), u(b.data+i/ubits), val(b.start+i), bits(*u) {}

		item const operator * () const {return val;}

		void operator ++ ()
		{ do {	if (!(bits >>= 1))
			     {	val += ubits-bit;
				bit  = 0;
				bits = *++u;
			     }
			else {	++bit;
				++val;
			     }
		     }	while (!(bits&1));
		}

		bool operator != (iterator& other) const {return val != other.val;}
	};

	iterator begin() const
	{	iterator it(*this, 0);
		if (!(it.bits&1)) ++it;
		return it;
	}

	iterator end() const {return iterator(*this, len);}
};

template <class item> struct TMBImpl<item,uint32_t>
{	static void bitwise_or(TMBitset<item,uint32_t>& a, const TMBitset<item,uint32_t>& b)
	{	uint64_t *end = (uint64_t*)(a.data+a.units)-1;
		uint64_t *d64 = (uint64_t*)a.data;
		uint64_t *o64 = (uint64_t*)b.data;
		while (d64 <= end) *d64++ |= *o64++;
		if (a.units & 1) *(uint32_t*)d64 |= *(uint32_t*)o64;
	}
};
