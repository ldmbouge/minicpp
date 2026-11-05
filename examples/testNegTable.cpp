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
 * Tests for NegTableCT: (x0,...,xk) differs from every row of the table
 */

#undef NDEBUG   // keep asserts in Release builds
#include <iostream>
#include <cassert>
#include "solver.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "table.hpp"
#include "search.hpp"

using Table = std::vector<std::vector<int>>;

// every tuple with x0 = 0 is forbidden, so 0 leaves D(x0)
int testFullRow() {
   using namespace Factory;
   CPSolver::Ptr cp = makeSolver();
   auto x = intVarArray(cp, 2, 0, 1);
   cp->post(negTable(x, Table{{0,0},{0,1}}));
   assert(x[0]->isBound() && x[0]->min() == 1);
   assert(x[1]->size() == 2);
   cp.dealloc();
   return 0;
}

// duplicate rows must not be counted twice
int testDuplicates() {
   using namespace Factory;
   CPSolver::Ptr cp = makeSolver();
   auto x = intVarArray(cp, 2, 0, 1);
   cp->post(negTable(x, Table{{0,0},{0,0}}));
   assert(x[0]->size() == 2 && x[1]->size() == 2);
   cp.dealloc();
   return 0;
}

// a partly forbidden value stays until the other domains shrink
int testShrink() {
   using namespace Factory;
   CPSolver::Ptr cp = makeSolver();
   auto x = intVarArray(cp, 3, 0, 2);
   cp->post(negTable(x, Table{{0,0,0},{0,0,1},{0,1,0}}));
   assert(x[0]->size() == 3);
   cp->post(x[1] == 0);
   assert(x[0]->size() == 3);
   cp->post(x[2] <= 1);
   assert(!x[0]->contains(0));
   cp.dealloc();
   return 0;
}

// rows using values outside the domains are ignored
int testOutOfDomain() {
   using namespace Factory;
   CPSolver::Ptr cp = makeSolver();
   auto x = intVarArray(cp, 2, 0, 1);
   cp->post(negTable(x, Table{{5,0},{5,1},{0,7}}));
   assert(x[0]->size() == 2 && x[1]->size() == 2);
   cp.dealloc();
   return 0;
}

// forbid every tuple summing to 3: 27 - 7 = 20 solutions remain
int testEnum() {
   using namespace Factory;
   CPSolver::Ptr cp = makeSolver();
   auto x = intVarArray(cp, 3, 0, 2);
   Table t;
   for (int a = 0; a <= 2; a++)
      for (int b = 0; b <= 2; b++)
         for (int c = 0; c <= 2; c++)
            if (a + b + c == 3)
               t.push_back({a,b,c});
   cp->post(negTable(x, t));
   DFSearch search(cp, firstFail(cp, x));
   search.onSolution([&]() {
      assert(x[0]->min() + x[1]->min() + x[2]->min() != 3);
   });
   auto stat = search.solve();
   assert(stat.numberOfSolutions() == 20);
   std::cout << stat << std::endl;
   cp.dealloc();
   return 0;
}

// more than 64 rows exercises the multi-word bitsets
int testManyRows() {
   using namespace Factory;
   CPSolver::Ptr cp = makeSolver();
   auto x = intVarArray(cp, 3, 0, 4);
   Table t;
   for (int b = 0; b <= 4; b++)          // x0 = 4 fully forbidden (25 rows)
      for (int c = 0; c <= 4; c++)
         t.push_back({4,b,c});
   for (int a = 0; a <= 3; a++)          // plus 60 rows with x2 != 0
      for (int b = 0; b <= 4; b++)
         for (int c = 1; c <= 3; c++)
            t.push_back({a,b,c});
   cp->post(negTable(x, t));
   assert(x[0]->max() == 3);
   DFSearch search(cp, firstFail(cp, x));
   auto stat = search.solve();
   assert(stat.numberOfSolutions() == 125 - 85);
   cp.dealloc();
   return 0;
}

int main() {
   testFullRow();
   testDuplicates();
   testShrink();
   testOutOfDomain();
   testEnum();
   testManyRows();
   std::cout << "testNegTable: all passed" << std::endl;
   return 0;
}
