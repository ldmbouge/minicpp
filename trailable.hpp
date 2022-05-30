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
 */

#ifndef __TRAILABLE_H
#define __TRAILABLE_H

#include "trail.hpp"

/**
 * @brief This template class is meant to represent values of type `T` that can be preserved
 * on write and restored on backtrack.

 * It supports common-place numerical operators and uses the Trailer::Ptr it gets when created
 * to record changes on write. This is the architecture used in MiniCP to provide *trailing*.
 * @param `T` the type of numerical value to be trailed.
 */
template<class T> class trail {
   Trailer::Ptr       _ctx;
   int              _magic;
   T                _value;
   void save(int nm) {
      _magic = nm;
      Entry* entry = new (_ctx) TrailEntry(&_value);
      _ctx->trail(entry);
   }
public:
   /**
    * Default constructor to build an array of uninitialised  trailable values.
    */ 
   trail() : _ctx(nullptr),_magic(-1),_value(T())  {}
   /**
    * @brief Constructor.
    *
    * @param ctx the trailer to use from this point forward.
    * @param v the initial value to assign to this object
    */
   trail(Trailer::Ptr ctx,const T& v = T()) : _ctx(ctx),_magic(ctx->magic()),_value(v) {}
   /**
    * Reports whether the value held in the object was changed since the last choice
    * @return true if the magic number is *stale* (the trailable was not written to yet in this choice point).
    */
   bool fresh() const { return _magic != _ctx->magic();}
   /**
    * Reports the Trailer::Ptr object used by this value
    * @return a pointer to the trailer
    */
   Trailer::Ptr ctx() const { return _ctx;}
   /**
    * @brief Casting operator that returns the current value of the object.
    * @return Current value of the object.
    */
   operator T() const { return _value;}
   /**
    * @brief Value method that returns the current value of the object.
    * @return Current value of the object. 
    */
   T value() const { return _value;}
   /**
    * Assignment operator
    * @param v the new value to record
    * @return the object itself y (after the operation is done).
    */
   trail<T>& operator=(const T& v);
   /**
    * Increment operator
    * @param v the new value to add to the current one.
    * @return the object itself  (after the operation is done).
    */
   trail<T>& operator+=(const T& v);
   /**
    * Decrement operator
    * @param v the new value to subtract from the current one.
    * @return the object itself (after the operation is done).
    */
   trail<T>& operator-=(const T& v);
   /**
    * Pre-increment by 1
    * @return the object after the increment.
    */ 
   trail<T>& operator++(); // pre-increment
   /**
    * Post-increment by 1
    * @return the value prior to the increment.
    */
   T operator++(int); // post-increment
   /**
    * Post-decrement by 1
    * @return the value prior to the increment.
    */   
   T operator--(int); // post-decrement
   /**
    * @brief Nested class to record the old value when a write occurs.
    * An instance of this class is to be written on the trail each time
    * the object modifies the value it represents.
    */   
   class TrailEntry: public Entry {
      T*  _at;
      T  _old;
   public:
      TrailEntry(T* at) : _at(at),_old(*at) {}
      void restore() noexcept { *_at = _old;}
   };
};

template<class T>
trail<T>& trail<T>::operator=(const T& v)
{
   int cm = _ctx->magic();
   if (_magic != cm)
      save(cm);
   _value = v;
   return *this;
}

template <class T>
trail<T>& trail<T>::operator++() { // pre-increment
   int cm = _ctx->magic();
   if (_magic != cm)
      save(cm);
   _value += 1;
   return *this;
}

template <class T>
T trail<T>::operator++(int) { // post-increment
   T rv = _value;
   int cm = _ctx->magic();
   if (_magic != cm)
      save(cm);
   ++_value;
   return rv;
}

template <class T>
T trail<T>::operator--(int) { // post-increment
   T rv = _value;
   int cm = _ctx->magic();
   if (_magic != cm)
      save(cm);
   --_value;
   return rv;
}


template<class T>
trail<T>& trail<T>::operator+=(const T& v) {
   int cm = _ctx->magic();
   if (_magic != cm)
      save(cm);
   _value += v;
   return *this;
}

template<class T>
trail<T>& trail<T>::operator-=(const T& v) {
   int cm = _ctx->magic();
   if (_magic != cm)
      save(cm);
   _value -= v;
   return *this;
}

#endif
