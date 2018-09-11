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
   std::cout << "Segment(" << _sz << ") deallocated" <<  std::endl;
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
