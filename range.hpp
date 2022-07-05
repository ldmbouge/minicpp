/*
 * mini-cp is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License  v3
 * as published by the Free Software Foundation.
 *
 * mini-cp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY.
 * See the GNU Lesser General Public License  for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with mini-cp. If not, see http://www.gnu.org/licenses/lgpl-3.0.en.html
 *
 * Copyright (c)  2018. by Laurent Michel, Pierre Schaus, Pascal Van Hentenryck
 *
 * Contributions by Waldemar Cruz, Rebecca Gentzel, Willem Jan Van Hoeve
 */

#ifndef __RANGE_HPP
#define __RANGE_HPP

#include <tuple>

template <typename T> class Range {  // close range [from,to]
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
   typedef T value_type;
   Range(T f,T t) : _from(f),_to(t) {} // [from,to]
   iterator begin() const { return iterator(_from); }
   iterator end() const   { return iterator(_to+1); } // end is always excluded
   T first() const { return _from;}
   T to() const { return _to;}
   int size() const { return _to - _from+1;} // size of a closed range
};

template <typename VT> const Range<VT> range(VT from,VT to)
{
   return Range<VT>(from,to);
}

#endif
