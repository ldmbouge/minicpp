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
 * Tests for StaticBitSet and SparseBitSet with 32- and 64-bit words
 */

#undef NDEBUG   // keep asserts in Release builds
#include <iostream>
#include <cassert>
#include <cstdint>
#include "bitset.hpp"
#include "solver.hpp"

// 70 bits spans several words whatever the width, the last one partly used
const int SZ = 70;

template <class W> StaticBitSet<W> evens() {
   StaticBitSet<W> bs(SZ);
   for (int i = 1; i < SZ; i += 2)
      bs.remove(i);
   return bs;
}

template <class W> StaticBitSet<W> multiplesOf3() {
   StaticBitSet<W> bs(SZ);
   for (int i = 0; i < SZ; i++)
      if (i % 3)
         bs.remove(i);
   return bs;
}

template <class W> void testCount() {
   StaticBitSet<W> full(SZ);
   assert(full.count() == SZ);       // unused bits of the last word are off
   for (int i = 0; i < SZ; i++)
      assert(full.contains(i));
   full.remove(0);
   full.remove(69);
   assert(full.count() == SZ - 2);
   assert(!full.contains(0) && !full.contains(69) && full.contains(1) && full.contains(68));
   full.clear();
   assert(full.count() == 0);
   StaticBitSet<W> empty(0);
   assert(empty.count() == 0);
   StaticBitSet<W> exact(sizeof(W) * 8 * 2); // exactly two full words
   assert(exact.count() == (int)sizeof(W) * 8 * 2);
}

template <class W> void testIntersectCount() {
   const StaticBitSet<W> e = evens<W>();
   const StaticBitSet<W> m = multiplesOf3<W>();
   assert(e.count() == 35);
   assert(m.count() == 24);          // 0,3,...,69
   assert(e.intersectCount(m) == 12); // multiples of 6: 0,6,...,66
   assert(m.intersectCount(e) == 12);
   assert(e.count() == 35);          // operands are untouched
}

template <class W> void testUnionIntersect() {
   StaticBitSet<W> u = evens<W>();
   u.unionWith(multiplesOf3<W>());
   assert(u.count() == 35 + 24 - 12);
   assert(u.contains(3) && u.contains(4) && !u.contains(5));
   StaticBitSet<W> n = evens<W>();
   n.intersectWith(multiplesOf3<W>());
   assert(n.count() == 12);
   assert(n.contains(66) && !n.contains(3) && !n.contains(4));
   StaticBitSet<W> z(SZ);
   z.clear();
   z.unionWith(evens<W>());          // union into an empty set copies
   assert(z.count() == 35 && z.intersectCount(evens<W>()) == 35);
}

template <class W> void testConstIndex() {
   const StaticBitSet<W> e = evens<W>();
   StaticBitSet<W> copy(SZ);
   copy.clear();
   copy.unionWith(e);
   const int nbWords = (SZ + sizeof(W) * 8 - 1) / (sizeof(W) * 8);
   for (int w = 0; w < nbWords; w++)
      assert(e[w] == copy[w]);
}

// SparseBitSet: masks, intersections and backtracking
template <class W> void testSparse() {
   CPSolver::Ptr cp = Factory::makeSolver();
   auto sm = cp->getStateManager();
   sm->enable();                      // the trailer records nothing until enabled (search does it)
   SparseBitSet<W> s(sm, cp->getStore(), SZ);
   assert(!s.isEmpty());
   const StaticBitSet<W> e = evens<W>();
   assert(s.intersectIndex(e) == 0);
   sm->saveState();
   s.clearMask();
   s.addToMask(e);
   s.intersectWithMask();             // s = evens
   const StaticBitSet<W> m = multiplesOf3<W>();
   assert(s.intersectIndex(m) == 0);  // 0 is in both
   StaticBitSet<W> odds(SZ);
   for (int i = 0; i < SZ; i += 2)
      odds.remove(i);
   assert(s.intersectIndex(odds) == -1);
   s.clearMask();
   s.addToMask(odds);
   s.intersectWithMask();             // evens & odds = {}
   assert(s.isEmpty());
   sm->restoreState();
   assert(!s.isEmpty());              // back to the full set
   assert(s.intersectIndex(odds) != -1); // word order may be permuted, so only existence is checked
   cp.dealloc();
}

template <class W> StaticBitSet<W> single(int sz,int k) {
   StaticBitSet<W> bs(sz);
   for (int i = 0; i < sz; i++)
      if (i != k)
         bs.remove(i);
   return bs;
}

// clearBit must find a word even after an emptied word was swapped out of the way
template <class W> void testSparseClearBit() {
   const int B = sizeof(W) * 8;
   CPSolver::Ptr cp = Factory::makeSolver();
   auto sm = cp->getStateManager();
   {  // emptying word 0 must not hide word 2 from clearBit
      SparseBitSet<W> s(sm, cp->getStore(), 3 * B);
      for (int b = 0; b < B; b++)
         s.clearBit(b);
      s.clearBit(2 * B + 1);
      assert(s.intersectIndex(single<W>(3 * B, 2 * B + 1)) == -1);
      assert(s.intersectIndex(single<W>(3 * B, 2 * B + 2)) != -1);
      assert(s.intersectIndex(single<W>(3 * B, B + 1)) != -1);
   }
   {  // clearing a bit of an emptied word must not touch another word
      SparseBitSet<W> s(sm, cp->getStore(), 3 * B);
      for (int b = B; b < 2 * B; b++)
         s.clearBit(b);
      s.clearBit(B + 2);
      assert(s.intersectIndex(single<W>(3 * B, 2 * B + 2)) != -1);
      assert(s.intersectIndex(single<W>(3 * B, 2)) != -1);
   }
   cp.dealloc();
}

template <class W> void testAll(const char* name) {
   testCount<W>();
   testIntersectCount<W>();
   testUnionIntersect<W>();
   testConstIndex<W>();
   testSparse<W>();
   testSparseClearBit<W>();
   std::cout << "  " << name << ": passed" << std::endl;
}

int main() {
   static_assert(std::is_same<StaticBitSet<>::Word, uint64_t>::value, "default word is 64-bit");
   static_assert(std::is_same<SparseBitSet<>::Word, uint64_t>::value, "default word is 64-bit");
   testAll<int>("int");
   testAll<uint32_t>("uint32_t");
   testAll<long>("long");
   testAll<uint64_t>("uint64_t");
   std::cout << "testBitSet: all passed" << std::endl;
   return 0;
}
