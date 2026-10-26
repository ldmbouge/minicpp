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
 * Tests for StaticBitSet: count, intersectCount, unionWith, intersectWith
 */

#undef NDEBUG   // keep asserts in Release builds
#include <iostream>
#include <cassert>
#include "bitset.hpp"

// 70 bits spans three 32-bit words, the last one partly used
const int SZ = 70;

StaticBitSet evens() {
   StaticBitSet bs(SZ);
   for (int i = 1; i < SZ; i += 2)
      bs.remove(i);
   return bs;
}

StaticBitSet multiplesOf3() {
   StaticBitSet bs(SZ);
   for (int i = 0; i < SZ; i++)
      if (i % 3)
         bs.remove(i);
   return bs;
}

int testCount() {
   StaticBitSet full(SZ);
   assert(full.count() == SZ);       // unused bits of the last word are off
   full.remove(0);
   full.remove(69);
   assert(full.count() == SZ - 2);
   full.clear();
   assert(full.count() == 0);
   StaticBitSet empty(0);
   assert(empty.count() == 0);
   return 0;
}

int testIntersectCount() {
   const StaticBitSet e = evens();
   const StaticBitSet m = multiplesOf3();
   assert(e.count() == 35);
   assert(m.count() == 24);          // 0,3,...,69
   assert(e.intersectCount(m) == 12); // multiples of 6: 0,6,...,66
   assert(m.intersectCount(e) == 12);
   assert(e.count() == 35);          // operands are untouched
   return 0;
}

int testUnionIntersect() {
   StaticBitSet u = evens();
   u.unionWith(multiplesOf3());
   assert(u.count() == 35 + 24 - 12);
   assert(u.contains(3) && u.contains(4) && !u.contains(5));
   StaticBitSet n = evens();
   n.intersectWith(multiplesOf3());
   assert(n.count() == 12);
   assert(n.contains(66) && !n.contains(3) && !n.contains(4));
   StaticBitSet z(SZ);
   z.clear();
   z.unionWith(evens());             // union into an empty set copies
   assert(z.count() == 35 && z.intersectCount(evens()) == 35);
   return 0;
}

int testConstIndex() {
   const StaticBitSet e = evens();
   StaticBitSet copy(SZ);
   copy.clear();
   copy.unionWith(e);
   for (int w = 0; w < 3; w++)
      assert(e[w] == copy[w]);
   return 0;
}

int main() {
   testCount();
   testIntersectCount();
   testUnionIntersect();
   testConstIndex();
   std::cout << "testBitSet: all passed" << std::endl;
   return 0;
}
