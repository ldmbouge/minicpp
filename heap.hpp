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

#ifndef __MINICPP_HEAP_H
#define __MINICPP_HEAP_H

#include <algorithm>
#include <iostream>
#include <functional>
#include "store.hpp"

/**
 * @brief A generic heap data structure.
 * 
 * This class implements a heap data structure that can be used to efficiently
 * manage and retrieve elements based on a specified order.
 * 
 * @tparam T The type of elements stored in the heap.
 * @tparam Ord The ordering function used to compare elements (default is std::greater).
 */
template <class T, class Ord = std::greater<T>> class Heap {
   Pool::Ptr _pool;
   Ord _ord;
   T* _tab;
   int _mxs;
   int  _at;

   /**
    * @brief Reorders the heap to maintain the heap property.
    * 
    * This function is used to restore the heap property after an insertion or
    * extraction operation.
    * 
    * @param p The index of the element to heapify.
    */
   void heapify(int p) noexcept {
      while(true) {
         int l = p * 2, r = p * 2 + 1;
         int largest;
         if (l < _at && _ord(_tab[l], _tab[p]))
            largest = l;
         else largest = p;
         if (r < _at && _ord(_tab[r], _tab[largest]))
            largest = r;
         if (largest != p) {
            std::swap(_tab[p], _tab[largest]);
            p = largest;
         } else break;
      }
   }

public:
   /**
    * @brief Constructs a new heap.
    * 
    * @param p A pointer to the memory pool used for allocation.
    * @param sz The initial size of the heap.
    */
   Heap(Pool::Ptr p, int sz) : _pool(p) {
      _mxs = sz;
      _at = 1;
      _tab = new (_pool) T[_mxs];
      for(int i = 0; i < _mxs; ++i) _tab[i] = T();
   }

   /**
    * @brief Clears the heap.
    * 
    * This function resets the heap, making it empty.
    */
   void clear() noexcept { _at = 1; }

   /**
    * @brief Returns the number of elements in the heap.
    * 
    * @return The number of elements in the heap.
    */
   unsigned size() const noexcept { return (unsigned)(_at - 1); }

   /**
    * @brief Checks if the heap is empty.
    * 
    * @return True if the heap is empty, false otherwise.
    */
   bool empty() const noexcept { return _at == 1; }

   /**
    * @brief Accesses an element in the heap.
    * 
    * @param i The index of the element to access.
    * @return A constant reference to the element at index i.
    */
   const T& operator[](int i) const noexcept { return _tab[i + 1]; }

   /**
    * @brief Inserts an element into the heap.
    * 
    * This function inserts an element into the heap and resizes the heap if necessary.
    * 
    * @param v The element to insert.
    */
   void insert(const T& v) noexcept {
      if (_at >= _mxs) {
         T* nt = new (_pool) T[_mxs << 1];
         for(int i = 0; i < _mxs; ++i) nt[i] = _tab[i];
         _tab = nt;
         _mxs <<= 1;
      }
      _tab[_at++] = v;
   }

   /**
    * @brief Inserts an element into the heap and maintains the heap property.
    * 
    * This function inserts an element into the heap and then calls heapify to
    * restore the heap property.
    * 
    * @param v The element to insert.
    */
   void insertHeap(const T& v) noexcept {
      insert(v);
      heapify(_at - 1);
   }

   /**
    * @brief Builds the heap from the current elements.
    * 
    * This function rearranges the elements in the heap to satisfy the heap property.
    */
   void buildHeap() noexcept {
      for(int i = _at / 2; i > 0; --i)
         heapify(i);
   }

   /**
    * @brief Extracts the maximum element from the heap.
    * 
    * This function removes and returns the maximum element from the heap.
    * 
    * @return The maximum element in the heap.
    */
   T extractMax() noexcept {
      T rv = _tab[1];
      _tab[1] = _tab[--_at];
      heapify(1);
      return rv;
   }
};

#endif
