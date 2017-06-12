#ifndef __STORE_H
#define __STORE_H

#include <vector>
#include "reversible.hpp"
#include "trail.hpp"

class Storage {
   struct Segment {
      char*      _base;
      std::size_t  _sz;
      Segment(std::size_t tsz);
      ~Segment();
      typedef std::shared_ptr<Segment> Ptr;
   };
   Context::Ptr                         _ctx;
   std::vector<Storage::Segment::Ptr> _store;
   rev<int>    _seg;
   rev<size_t> _top;   
public:
   Storage(Context::Ptr ctx); 
   ~Storage();
   typedef std::shared_ptr<Storage> Ptr;
   void* alloc(std::size_t sz);   
};


#endif
