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
#include "regular.hpp"
#include "search.hpp"

int main() {
   using namespace std;
   using namespace Factory;
   CPSolver::Ptr cp  = Factory::makeSolver();

   int n = 10;
   auto q = Factory::intVarArray(cp,n,1,10); // 10 vars (0..9) with domains 1..10
   auto tf = std::vector<Transition> {{1,1,2},{2,2,3},{3,2,4},{3,3,3},{4,1,5}};
   auto a = Factory::automaton(cp,1,10,1,5,1,{5},tf);
   
   //cp->post(q[1]!=2); // that would make it infeasible.
   cp->post(Factory::regular(q,a));
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
   cp.dealloc();
   return 0;
}
