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
 * Tests for Maximum: y = max(x0,...,xk)
 */

#undef NDEBUG   // keep asserts in Release builds
#include <iostream>
#include <cassert>
#include <algorithm>
#include "solver.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"

// y spans [max of mins, max of maxes]
int testRange() {
   using namespace Factory;
   CPSolver::Ptr cp = makeSolver();
   std::vector<var<int>::Ptr> x = {makeIntVar(cp, 0, 5), makeIntVar(cp, 2, 8), makeIntVar(cp, 1, 3)};
   auto y = maximum(x);
   assert(y->min() == 2 && y->max() == 8);
   cp->post(x[2] >= 3);
   assert(y->min() == 3);
   cp.dealloc();
   return 0;
}

// y.max caps every x[i]
int testCap() {
   using namespace Factory;
   CPSolver::Ptr cp = makeSolver();
   std::vector<var<int>::Ptr> x = {makeIntVar(cp, 0, 5), makeIntVar(cp, 2, 8), makeIntVar(cp, 1, 3)};
   auto y = maximum(x);
   cp->post(y <= 4);
   assert(x[0]->max() == 4 && x[1]->max() == 4 && x[2]->max() == 3);
   cp.dealloc();
   return 0;
}

// a single x[i] able to reach y.min must be the maximum
int testSingleSupport() {
   using namespace Factory;
   CPSolver::Ptr cp = makeSolver();
   std::vector<var<int>::Ptr> x = {makeIntVar(cp, 0, 3), makeIntVar(cp, 0, 10), makeIntVar(cp, 0, 2)};
   auto y = maximum(x);
   cp->post(y >= 5);
   assert(x[1]->min() == 5);
   assert(x[0]->max() == 3 && x[2]->max() == 2);
   cp.dealloc();
   return 0;
}

// every assignment gives y = max(x)
int testEnum() {
   using namespace Factory;
   CPSolver::Ptr cp = makeSolver();
   auto x = intVarArray(cp, 3, 0, 2);
   auto y = maximum(x);
   DFSearch search(cp, firstFail(cp, x));
   search.onSolution([&]() {
      assert(y->isBound());
      assert(y->min() == std::max({x[0]->min(), x[1]->min(), x[2]->min()}));
   });
   auto stat = search.solve();
   assert(stat.numberOfSolutions() == 27);
   std::cout << stat << std::endl;
   cp.dealloc();
   return 0;
}

int main() {
   testRange();
   testCap();
   testSingleSupport();
   testEnum();
   std::cout << "testMaximum: all passed" << std::endl;
   return 0;
}
