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

#ifndef __STORE_H
#define __STORE_H

#include <vector>
#include "handle.hpp"
#include "trail.hpp"
#include "trailable.hpp"
#include "stlAllocAdapter.hpp"

class Storage {
   struct Segment {
      char*      _base;
      std::size_t  _sz;
      Segment(std::size_t tsz);
      ~Segment();
      typedef std::shared_ptr<Segment> Ptr;
   };
   Trailer::Ptr                         _ctx;
   std::vector<Storage::Segment::Ptr> _store;
   trail<int>    _seg;
   trail<size_t> _top;   
public:
   Storage(Trailer::Ptr ctx); 
   ~Storage();
   typedef handle_ptr<Storage> Ptr;
   void* allocate(std::size_t sz);
   void free(void* ptr) {}
   std::size_t capacity() const;
};

inline void* operator new(std::size_t sz,Storage::Ptr store)
{
   return store->allocate(sz);
}

inline void* operator new[](std::size_t sz,Storage::Ptr store)
{
   return store->allocate(sz);
}

#endif
