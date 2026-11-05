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
 * Tests for TableCT: (x0,...,xk) equals some row of the table
 */

#undef NDEBUG   // keep asserts in Release builds
#include <iostream>
#include <cassert>
#include <set>
#include "solver.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "table.hpp"
#include "search.hpp"

using Table = std::vector<std::vector<int>>;

// allowed rows: every (a,b,c) in 0..6 with (a + 2b + 3c) % 4 == 1, which is
// well over 64 rows, so the tuple bitsets span several 64-bit words
int testEnum() {
   using namespace Factory;
   CPSolver::Ptr cp = makeSolver();
   auto x = intVarArray(cp, 3, 0, 6);
   Table t;
   for (int a = 0; a <= 6; a++)
      for (int b = 0; b <= 6; b++)
         for (int c = 0; c <= 6; c++)
            if ((a + 2*b + 3*c) % 4 == 1)
               t.push_back({a,b,c});
   assert(t.size() > 64);
   cp->post(table(x, t));
   std::set<std::vector<int>> seen;
   DFSearch search(cp, firstFail(cp, x));
   search.onSolution([&]() {
      std::vector<int> s = {x[0]->min(), x[1]->min(), x[2]->min()};
      assert((s[0] + 2*s[1] + 3*s[2]) % 4 == 1);
      seen.insert(s);
   });
   auto stat = search.solve();
   assert(stat.numberOfSolutions() == (int)t.size());
   assert(seen.size() == t.size());
   std::cout << "rows=" << t.size() << " " << stat << std::endl;
   cp.dealloc();
   return 0;
}

// supports vanish as domains shrink
int testPrune() {
   using namespace Factory;
   CPSolver::Ptr cp = makeSolver();
   auto x = intVarArray(cp, 2, 0, 3);
   cp->post(table(x, Table{{0,1},{1,2},{2,3},{3,0}}));
   assert(x[0]->size() == 4);
   cp->post(x[1] >= 2);
   assert(x[0]->size() == 2 && x[0]->contains(1) && x[0]->contains(2));
   cp.dealloc();
   return 0;
}

// Rows using values outside the domains are dropped when posting. Here the
// first 64 rows are all dropped, which empties a whole 64-bit word; the
// out-of-domain row (5,1) after it must still be dropped, or it wrongly
// supports x1 = 1 and (0,1) is accepted as a solution.
int testOutOfDomainRows() {
   using namespace Factory;
   CPSolver::Ptr cp = makeSolver();
   auto x = intVarArray(cp, 2, 0, 1);
   cp->post(x[0] == 0);
   Table t;
   for (int i = 0; i < 64; i++)
      t.push_back({5,0});         // word 0: all out of domain
   for (int i = 0; i < 64; i++)
      t.push_back({0,0});         // word 1: the only allowed assignment
   t.push_back({5,1});            // word 2: out of domain
   cp->post(table(x, t));
   assert(x[1]->isBound() && x[1]->min() == 0);
   DFSearch search(cp, firstFail(cp, x));
   auto stat = search.solve();
   assert(stat.numberOfSolutions() == 1);
   cp.dealloc();
   return 0;
}

int main() {
   testPrune();
   testEnum();
   testOutOfDomainRows();
   std::cout << "testTable: all passed" << std::endl;
   return 0;
}
