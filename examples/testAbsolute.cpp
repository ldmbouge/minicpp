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
 * Tests for Absolute: y = |x|
 */

#undef NDEBUG   // keep asserts in Release builds
#include <iostream>
#include <cassert>
#include "solver.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"

// the fresh variable covers the full range of |x|
int testRange() {
   using namespace Factory;
   CPSolver::Ptr cp = makeSolver();
   auto x1 = makeIntVar(cp, -5, 5);
   auto y1 = Factory::abs(x1);
   assert(y1->min() == 0 && y1->max() == 5);
   auto x2 = makeIntVar(cp, -10, 2);
   auto y2 = Factory::abs(x2);
   assert(y2->min() == 0 && y2->max() == 10);
   cp.dealloc();
   return 0;
}

// x of fixed sign: bounds flow both ways
int testSigned() {
   using namespace Factory;
   CPSolver::Ptr cp = makeSolver();
   auto xp = makeIntVar(cp, 2, 8);
   auto yp = Factory::abs(xp);
   assert(yp->min() == 2 && yp->max() == 8);
   cp->post(yp <= 5);
   assert(xp->max() == 5);
   auto xn = makeIntVar(cp, -8, -2);
   auto yn = Factory::abs(xn);
   assert(yn->min() == 2 && yn->max() == 8);
   cp->post(yn >= 4);
   assert(xn->max() == -4);
   cp.dealloc();
   return 0;
}

// y fixed leaves x in {-y, y}
int testFixedY() {
   using namespace Factory;
   CPSolver::Ptr cp = makeSolver();
   auto x = makeIntVar(cp, -5, 5);
   auto y = Factory::abs(x);
   cp->post(y == 3);
   assert(x->size() == 2 && x->contains(-3) && x->contains(3));
   cp.dealloc();
   return 0;
}

// x spans zero: y.min skips values with no support in x
int testMixed() {
   using namespace Factory;
   CPSolver::Ptr cp = makeSolver();
   auto x = makeIntVar(cp, -5, 5);
   auto y = Factory::abs(x);
   cp->post(x != 0);
   cp->post(x != 1);
   cp->post(x != -1);
   assert(y->min() == 2);
   cp->post(y <= 4);
   assert(x->min() == -4 && x->max() == 4);
   cp.dealloc();
   return 0;
}

// every x gives y = |x|
int testEnum() {
   using namespace Factory;
   CPSolver::Ptr cp = makeSolver();
   auto xs = intVarArray(cp, 1, -3, 3);
   auto y = Factory::abs(xs[0]);
   DFSearch search(cp, firstFail(cp, xs));
   search.onSolution([&]() {
      assert(y->isBound());
      assert(y->min() == std::abs(xs[0]->min()));
   });
   auto stat = search.solve();
   assert(stat.numberOfSolutions() == 7);
   std::cout << stat << std::endl;
   cp.dealloc();
   return 0;
}

int main() {
   testRange();
   testSigned();
   testFixedY();
   testMixed();
   testEnum();
   std::cout << "testAbsolute: all passed" << std::endl;
   return 0;
}
