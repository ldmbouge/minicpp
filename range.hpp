#ifndef __RANGE_HPP
#define __RANGE_HPP

#include <tuple>

template <typename T> class Range {
   T _from,_to;
public:
   class iterator {
      friend class Range<T>;
      T _i;
   protected:
      iterator(T start) : _i(start) {}
   public:
      T operator *() const  { return _i; }
      const iterator& operator++() { ++_i; return *this; } // pre-increment
      iterator operator ++(int) { iterator copy(*this); ++_i; return copy; } // post-increment
      bool operator ==(const iterator& other) const { return _i == other._i; }
      bool operator !=(const iterator& other) const { return _i != other._i; }
   };
   Range(T f,T t) : _from(f),_to(t) {}
   iterator begin() const { return iterator(_from); }
   iterator end() const   { return iterator(_to); }
   T first() const { return _from;}
   T to() const { return _to;}
   int size() const { return _to - _from;}
};

template <typename VT> const Range<VT> range(VT from,VT to)
{
   return Range<VT>(from,to);
}

#endif
