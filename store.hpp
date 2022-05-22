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
/**
 * Default segment size for storage management (4 Megs).
 */
#define SEGSIZE (1 << 22)

/**
 * @brief Memory pool for the solver.
 *
 * The class is a segmented (and backtrackable) memory pool.
 * Objects are allocated on the pool with a stack like policy (LIFO).
 * When the solver _backtracks_ the stack allocator automatically
 * shrinks to restore the state to what was current at the point we are
 * returning to.
 */
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
   const std::size_t   _segSize;
   trail<size_t>           _top;   
   trail<unsigned>         _seg;
public:
   /**
    * @brief Create a pool managed by a Trailer::Ptr object.
    *
    * @param ctx the Trailer::Ptr instance that governs deallocation
    * @param defSize the size to use for each _segment_ that will have to be allocated
    * 
    * Note that when a segment is full, the allocator automatically grabs another segment
    * to serve the latest allocation request. Therefore, the allocator keeps a vector of
    * Segments that is shrunk/reused as needed on backtrack.
    */
   Storage(Trailer::Ptr ctx,std::size_t defSize = SEGSIZE); 
   ~Storage();
   typedef handle_ptr<Storage> Ptr;
   /**
    * @brief Grabs a block of memory large enough to hold the specified number of bytes.
    *
    * The address is aligned (8 bytes)
    * @param sz number of bytes
    * @return a pointer to a block large enough for `sz` bytes.
    */ 
   void* allocate(std::size_t sz);
   /**
    * @brief There is no freeing on a stack allocator. Only backtracking
    */
   void free(void* ptr) {}
   /**
    * Usage statistics
    * @return the maximum capacity the allocator can handle now.
    */
   std::size_t capacity() const;
   /**
    * Usage statistics
    * @return The segment size used to create this allocator.
    */
   std::size_t usage() const;
};

/**
 * @brief C++ new allocator that delegate to the given store. This allows us to use placement like new syntax.
 * @param sz how much space to allocate
 * @param store on which allocator to allocate
 */
inline void* operator new(std::size_t sz,Storage::Ptr store)
{
   return store->allocate(sz);
}

/**
 * @brief C++ new[] allocator that delegate to the given store. This allows us to use placement like new syntax for arrays.
 * @param sz how much space to allocate
 * @param store on which allocator to allocate
 */
inline void* operator new[](std::size_t sz,Storage::Ptr store)
{
   return store->allocate(sz);
}

class Pool;
/**
 * @brief Abstract object representing a mark. Nothing is user visible
 * @see Pool
 */
class PoolMark {
   friend class Pool;
   size_t   _top;
   unsigned _seg;
   PoolMark(size_t top,unsigned seg) : _top(top),_seg(seg) {}
};

/**
 * @brief Memory pool for the solver.
 *
 * The class is a segmented memory pool.
 * Objects are allocated on the pool with a stack like policy (LIFO).
 * There is no provision to automatically deallocate or shrink the pool.
 * Pool shrinkage is completely under the control of the programmer who
 * must take a `PoolMark` to remember the size of the allocator at some point
 * in time and later **clear** the allocator to restore its state to a chosen `PoolMark`.
 */
class Pool {
   struct Segment {
      char*      _base;
      std::size_t  _sz;
      Segment(std::size_t tsz) { _base = new char[tsz];_sz = tsz;}
      ~Segment()               { delete[] _base;}
      typedef std::shared_ptr<Segment> Ptr;
   };
   Segment** _store;
   const std::size_t   _segSize;
   size_t                  _top;
   unsigned                _seg;
   unsigned              _nbSeg;
   unsigned                _mxs;
public:
   Pool(std::size_t defSize = SEGSIZE);
   ~Pool();
   typedef handle_ptr<Pool> Ptr;
   void* allocate(std::size_t sz);
   void free(void* ptr) {}
   /**
    * @brief Restore the pool to its very initial state (fully empty).
    */
   void clear() { _top = 0;_seg = 0;}
   /**
    * @brief Restore the pool to a specified mark. One can only shrink the pool.
    * @param m the mark to restore to.
    */
   void clear(const PoolMark& m) { _top = m._top;_seg = m._seg;}
   /**
    * @brief Capture the current mark and return it.
    * @return a PoolMark instance referring to the size of the allocator now.
    */
   PoolMark mark() const noexcept { return {_top,_seg};}
   std::size_t capacity() const noexcept { return _segSize;}
   std::size_t usage() const noexcept { return (_nbSeg - 1) * _segSize  + _top;}
};

inline void* operator new(std::size_t sz,Pool::Ptr store)
{
   return store->allocate(sz);
}

inline void* operator new[](std::size_t sz,Pool::Ptr store)
{
   return store->allocate(sz);
}

#endif
