#ifndef TMBITSET
#define TMBITSET

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <utility>

template <class unit> struct TMBImpl;

template <class item, class unit> class TMBitset
{	size_t units;
	size_t len;
	item   start;
	unit * data;

	static constexpr size_t ubits = 8*sizeof(unit);
	static const unit null_datum; // constexpr inline under C++17

	public:
	TMBitset(): units(1), len(0), start(0), data((unit*)&null_datum) {}
	TMBitset(item const s, const size_t l) {alloc(s,l);}
	TMBitset(const TMBitset<item,unit>& other) {*this = other;}
	~TMBitset() {if (data != &null_datum) free(data);}

	// Does not free resources. Only call on
	// unallocated objects or manually free beforehand.
	void operator = (const TMBitset<item,unit>& other)
	{	units = other.units;
		len   = other.len;
		start = other.start;
		data  = (unit*)malloc(units*sizeof(unit));
		memcpy(data, other.data, units*sizeof(unit));
	}

	// Does not free resources. Only intended for
	// post-facto setup of default-constructed objects.
	void alloc(item const s, const size_t l)
	{	units = ceil(double(l+1)/ubits);
		len   = l;
		start = s;
		data  = (unit*)calloc(units, sizeof(unit));
		//create dummy end() item
		data[units-1] |= (unit)1 << len%ubits;
	}

	// Don't clear null sets; there's nothing to free.
	void clear()
	{	units=1; len=0; start=0;
		free(data);
		data=(unit*)&null_datum;
	}

	// No bounds checking. No testing for null sets.
	// Only to be used with properly allocated sets.
	bool add_value(item   const value) {return add_index(value-start);}
	bool add_index(size_t const index)
	{	const unit u = data[index/ubits];
		data[index/ubits] |= (unit)1 << index%ubits;
		return u != data[index/ubits];
	}

	// For use when both sets' start & len are known to match, e.g. HighwaySegmwent::clinched_by
	void fast_union(const TMBitset<item,unit>& other) {TMBImpl<unit>::bitwise_oreq(data, other.data, units);}

	// return an end() index (one past the last 1 bit in the set) for shrink_to_fit()
	size_t end_index() const
	{	long u = units-1;
		if (unit bits = data[u] ^ unit(1) << len%ubits) // copy sans end() bit
			return u*ubits + 64 - __builtin_clzl(bits);
		while (--u >= 0)
		    if (data[u])
			return u*ubits + 64 - __builtin_clzl(data[u]);
		return 0;
	}

	void shrink_to_fit()
	{	if (!len) return;
		unit* const old_data = data;
		size_t lo_index = *ibegin() & ~(ubits-1);

		start += lo_index;
		len = end_index() - lo_index;
		units = ceil(double(len+1)/ubits);
		data = (unit*)malloc(units*sizeof(unit));

		memcpy(data, old_data+lo_index/ubits, units*sizeof(unit));
		free(old_data);
		//create dummy end() item
		data[units-1] |= (unit)1 << len%ubits;
	}

	bool operator [] (const size_t index) const {return data[index/ubits] & (unit)1 << index%ubits;}

	// The != operator assumes both objects have the same start & length, and shall only be used in this context.
	bool operator != (const TMBitset<item,unit>& other) const {return memcmp(data, other.data, units*sizeof(unit));}

	void operator |= (const TMBitset<item,unit>& b)
	{	if (!b.len) return;
		if (!len)
		{	if (data != &null_datum) free(data);
			*this = b;
			return;
		}
		item   const a_end = start + len;
		item   const b_end = b.start + b.len;
		size_t const old_l = len;
		size_t const old_u = units;
		unit * const old_d = data;
		size_t copy_offset, oreq_offset;

		if (start > b.start)
		     {	oreq_offset = start-b.start;
			if (oreq_offset % ubits) throw std::make_pair(this, &b);
			oreq_offset /= ubits;
			copy_offset = 0;
			start = b.start;
		     }
		else {	copy_offset = b.start-start;
			if (copy_offset % ubits) throw std::make_pair(this, &b);
			copy_offset /= ubits;
			oreq_offset = 0;
		     }
		len = (a_end > b_end ? a_end : b_end) - start;
		units = ceil(double(len+1)/ubits);
		data = (unit*)calloc(units, sizeof(unit));

		memcpy(data+copy_offset, b.data, b.units*sizeof(unit));
		if (a_end < b_end) // clear lower end() bit
			old_d[old_u-1] ^= (unit)1 << old_l%ubits;
		else	data[b.units+copy_offset-1] ^= (unit)1 << b.len%ubits;
		TMBImpl<unit>::bitwise_oreq(data+oreq_offset, old_d, old_u);
		free(old_d);
	}

	void operator &= (const TMBitset<item,unit>& b)
	{	if (!len)   return;
		if (!b.len) return clear();
		item   const a_end = start + len;
		item   const b_end = b.start + b.len;
		size_t const old_l = len;
		unit * const old_d = data;
		size_t copy_offset, band_offset;

		if (start < b.start)
		     {	band_offset = b.start-start;
			if (band_offset % ubits) throw std::make_pair(this, &b);
			band_offset /= ubits;
			copy_offset = 0;
			start = b.start;
		     }
		else {	copy_offset = start-b.start;
			if (copy_offset % ubits) throw std::make_pair(this, &b);
			copy_offset /= ubits;
			band_offset = 0;
		     }
		item new_end = a_end < b_end ? a_end : b_end;
		if ( new_end < start ) return clear();
		len = new_end - start;
		units = ceil(double(len+1)/ubits);
		data = (unit*)calloc(units, sizeof(unit));

		memcpy(data, b.data+copy_offset, units*sizeof(unit));
		if (a_end > b_end) // clear higher end() bit
			old_d[old_l/ubits] ^= (unit)1 << old_l%ubits;
		else if (b.units == units) // if it's in the new dataset's range
			   data[b.units-1] ^= (unit)1 << b.len%ubits;
		TMBImpl<unit>::bitwise_andeq(data, old_d+band_offset, units);
		free(old_d);
		//create dummy end() item
		data[units-1] |= (unit)1 << len%ubits;
	}

	template <class deref> class iterator
	{	size_t bit;
		unit* u;
		deref val;
		unit bits;

		friend iterator<item>	TMBitset::begin() const;
		friend iterator<size_t>	TMBitset::ibegin() const;

		public:
		iterator(unit* const d, deref const s, size_t const i): bit(i%ubits), u(d+i/ubits), val(s+i), bits(*u) {}

		deref const operator * () const {return val;}

		void operator ++ ()
		{ do {	if (!(bits &= ~(unit)1))
			     {	val += ubits-bit;
				bit  = 0;
				bits = *++u;
			     }
			else {	auto tz = __builtin_ctzl(bits);
				bit += tz;
				val += tz;
				bits >>= tz;
			     }
		     }	while (!(bits&1));
		}

		bool operator != (iterator& other) const {return val != other.val;}
	};

	iterator<item> begin() const
	{	iterator<item> it(data, start, 0);
		if (!(it.bits&1)) ++it;
		return it;
	}
	iterator<item> end() const {return iterator<item>(data, start, len);}

	iterator<size_t> ibegin() const
	{	iterator<size_t> it(data, 0, 0);
		if (!(it.bits&1)) ++it;
		return it;
	}
	iterator<size_t> iend() const {return iterator<size_t>(data, 0, len);}

	// diagnostics, debug, etc.
	bool is_null_set() {return data == &null_datum;}

	size_t count()
	{	size_t count = 0;
		for (unit *d = data, *e = d+units; d < e; ++d)
			count += __builtin_popcountl(*d);
		return	count-1;
	}

	size_t heap()	  {return sizeof(unit) * units;}
	size_t vec_size() {return sizeof(item) * count();}
	size_t vec_cap()
	{	size_t c = count();
		size_t res = c & size_t(-1) << (63-__builtin_clzl(c));
		if (c^res) res <<= 1;
		return sizeof(item) * res;
	}
};

// Not necessary under C++17; just use a constexpr
template <class item, class unit> const unit TMBitset<item, unit>::null_datum = 1;

template <> struct TMBImpl<uint32_t>
{	static void bitwise_oreq(uint32_t* a, const uint32_t* const b, size_t units)
	{	uint64_t *end = (uint64_t*)(a+units)-1;
		uint64_t *d64 = (uint64_t*)a;
		uint64_t *o64 = (uint64_t*)b;
		while (d64 <= end) *d64++ |= *o64++;
		if (units & 1) *(uint32_t*)d64 |= *(uint32_t*)o64;
	}
};

template <> struct TMBImpl<uint64_t>
{	static void bitwise_oreq(uint64_t* a, const uint64_t* b, size_t units)
	{	for (uint64_t *end = a+units; a < end; *a++ |= *b++);
	}
	static void bitwise_andeq(uint64_t* a, const uint64_t* b, size_t units)
	{	for (uint64_t *end = a+units; a < end; *a++ &= *b++);
	}
};
#endif
