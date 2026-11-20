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

/**
 * @brief Represents a closed range [from, to].
 * 
 * This class template represents a closed range of values from `from` to `to`.
 * It provides iterators to iterate over the range and methods to access the range's properties.
 * 
 * @tparam T The type of the values in the range.
 */
template <typename T> class Range {  
   T _from,_to;
public:
   /**
    * @brief Iterator class for Range.
    * 
    * This class provides iterators to iterate over the values in the range.
    */
   class iterator {
      friend class Range<T>;
      T _i;
   protected:
      /**
       * @brief Constructs an iterator starting at the given value.
       * 
       * @param start The starting value of the iterator.
       */
      iterator(T start) : _i(start) {}
   public:
      /**
       * @brief Dereferences the iterator.
       * 
       * @return The current value of the iterator.
       */
      T operator *() const  { return _i; }
      
      /**
       * @brief Pre-increments the iterator.
       * 
       * @return A reference to the iterator after incrementing.
       */
      const iterator& operator++() { ++_i; return *this; } // pre-increment
      
      /**
       * @brief Post-increments the iterator.
       * 
       * @return A copy of the iterator before incrementing.
       */
      iterator operator ++(int) { iterator copy(*this); ++_i; return copy; } // post-increment
      
      /**
       * @brief Checks if two iterators are equal.
       * 
       * @param other The other iterator to compare with.
       * @return True if the iterators are equal, false otherwise.
       */
      bool operator ==(const iterator& other) const { return _i == other._i; }
      
      /**
       * @brief Checks if two iterators are not equal.
       * 
       * @param other The other iterator to compare with.
       * @return True if the iterators are not equal, false otherwise.
       */
      bool operator !=(const iterator& other) const { return _i != other._i; }
   };
   
   /**
    * @brief The value type of the range.
    */
   typedef T value_type;
   
   /**
    * @brief Constructs a Range object.
    * 
    * @param f The starting value of the range.
    * @param t The ending value of the range.
    */
   Range(T f,T t) : _from(f),_to(t) {} // [from,to]
   
   /**
    * @brief Returns an iterator to the beginning of the range.
    * 
    * @return An iterator pointing to the first element in the range.
    */
   iterator begin() const { return iterator(_from); }
   
   /**
    * @brief Returns an iterator to the end of the range.
    * 
    * @return An iterator pointing to the element after the last element in the range.
    */
   iterator end() const   { return iterator(_to+1); } // end is always excluded
   
   /**
    * @brief Returns the first value in the range.
    * 
    * @return The first value in the range.
    */
   T first() const { return _from;}
   
   /**
    * @brief Returns the last value in the range.
    * 
    * @return The last value in the range.
    */
   T to() const { return _to;}
   
   /**
    * @brief Returns the size of the range.
    * 
    * @return The size of the range, which is the number of elements in the range.
    */
   int size() const { return _to - _from+1;} // size of a closed range
};

/**
 * @brief Creates a Range object.
 * 
 * @tparam VT The type of the values in the range.
 * @param from The starting value of the range.
 * @param to The ending value of the range.
 * @return A Range object representing the range [from, to].
 */
template <typename VT> const Range<VT> range(VT from,VT to)
{
   return Range<VT>(from,to);
}

#endif
