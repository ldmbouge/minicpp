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
 * Example Tim Curry
 */


#include <iostream>
#include <iomanip>
#include "bitset.hpp"
#include "solver.hpp"
#include "trailable.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "table.hpp"
#include "search.hpp"

int main() {
   using namespace std;
   using namespace Factory;
   CPSolver::Ptr cp  = Factory::makeSolver();

   int n = 3;
   auto q = Factory::intVarArray(cp,n,1,4);
   std::vector<std::vector<int>> table;
   table.emplace_back(std::vector<int> {1,2,3});
   table.emplace_back(std::vector<int> {3,2,1});
   table.emplace_back(std::vector<int> {2,1,4});
   cp->post(Factory::table(q, table));
   cp->post(q[1]!=2);
   //  cp->post(Factory::allDifferentAC(q));
   //  cp->post(q[0] == 4);
   DFSearch search(cp,[=]() {
                         auto x = selectMin(q,
                                            [](const auto& x) { return x->size() > 1;},
                                            [](const auto& x) { return x->size();});
                         if (x) {
                            int c = x->min();                    
                            return  [=] { cp->post(x == c);}
                               | [=] { cp->post(x != c);};
                         } else return Branches({});
                      });    
   search.onSolution([&q]() {
                        std::cout << "sol = " << q << std::endl;
                     });

   auto stat = search.solve();
   std::cout << stat << std::endl;
   //  std::cout << cp << std::endl;
   cp.dealloc();
   return 0;
}
