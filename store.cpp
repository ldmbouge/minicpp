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

#include "store.hpp"
#include <assert.h>

#define SEGSIZE (1 << 20)

Storage::Segment::Segment(std::size_t tsz)
{
   _sz = tsz;
   _base = new char[_sz];
}

Storage::Segment::~Segment()
{
   delete []_base;
   //std::cout << "Segment(" << _sz << ") deallocated" <<  std::endl;
}

Storage::Storage(Trailer::Ptr ctx)
   : _ctx(ctx),
     _store(0),
     _top(ctx,0),
     _seg(ctx,0)
{
   _store.push_back(std::make_shared<Storage::Segment>(SEGSIZE));
}

std::size_t Storage::capacity() const
{
   return SEGSIZE;
}

std::size_t Storage::usage() const
{
   return _store.size() * SEGSIZE;
}

Storage::~Storage()
{
   _store.clear();
}

void* Storage::allocate(std::size_t sz)
{
   if (sz & 7)  // unaligned on 8 bytes boundary
      sz = (sz | 7) + 1; // increase to align
   assert((sz & 7) == 0 && sz != 0);           // check alignment
   auto s = _store[_seg];
   if (_top + sz >= s->_sz) {
      if (_seg == _store.size() - 1)
         _store.push_back(std::make_shared<Storage::Segment>(SEGSIZE));
      _seg = _seg + 1;
      _top = 0;
      s = _store[_seg];
   }
   void* ptr = s->_base + _top;
   _top   = _top + sz;
   return ptr;
}
