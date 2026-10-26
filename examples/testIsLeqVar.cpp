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
 * Tests for IsLessOrEqualVar: b <=> x <= y
 */

#undef NDEBUG   // keep asserts in Release builds
#include <iostream>
#include <cassert>
#include "solver.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"

// b true prunes x <= y
int testTrue() {
   using namespace Factory;
   CPSolver::Ptr cp = makeSolver();
   auto x = makeIntVar(cp, 0, 10);
   auto y = makeIntVar(cp, 0, 5);
   auto b = makeBoolVar(cp);
   cp->post(new (cp) IsLessOrEqualVar(b, x, y));
   assert(!b->isBound());
   cp->post(b == true);
   assert(x->max() == 5);
   cp->post(y <= 3);
   assert(x->max() == 3);
   cp.dealloc();
   return 0;
}

// b false prunes x >= y + 1
int testFalse() {
   using namespace Factory;
   CPSolver::Ptr cp = makeSolver();
   auto x = makeIntVar(cp, 0, 5);
   auto y = makeIntVar(cp, 0, 5);
   auto b = makeBoolVar(cp);
   cp->post(new (cp) IsLessOrEqualVar(b, x, y));
   cp->post(b == false);
   assert(x->min() == 1);
   assert(y->max() == 4);
   cp.dealloc();
   return 0;
}

// bounds alone decide b
int testEntailed() {
   using namespace Factory;
   CPSolver::Ptr cp = makeSolver();
   auto x1 = makeIntVar(cp, 0, 3);
   auto y1 = makeIntVar(cp, 3, 8);
   auto b1 = isLessOrEqual(x1, y1);
   assert(b1->isTrue());
   auto x2 = makeIntVar(cp, 5, 8);
   auto y2 = makeIntVar(cp, 0, 4);
   auto b2 = isLessOrEqual(x2, y2);
   assert(b2->isFalse());
   cp.dealloc();
   return 0;
}

// every (x,y) pair fixes b to the right value
int testEnum() {
   using namespace Factory;
   CPSolver::Ptr cp = makeSolver();
   auto xy = intVarArray(cp, 2, 0, 4);
   auto b = isLessOrEqual(xy[0], xy[1]);
   DFSearch search(cp, firstFail(cp, xy));
   int nbTrue = 0;
   search.onSolution([&]() {
      assert(b->isBound());
      assert(b->isTrue() == (xy[0]->min() <= xy[1]->min()));
      nbTrue += b->isTrue();
   });
   auto stat = search.solve();
   assert(stat.numberOfSolutions() == 25);
   assert(nbTrue == 15);
   std::cout << stat << std::endl;
   cp.dealloc();
   return 0;
}

int main() {
   testTrue();
   testFalse();
   testEntailed();
   testEnum();
   std::cout << "testIsLeqVar: all passed" << std::endl;
   return 0;
}
