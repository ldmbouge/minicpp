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
   auto x = Factory::intVarArray(cp,n,1,50);
   cp->post(x[0] <= 10);
   cp->post(x[1] >= 30);
   cp->post(x[2] >= 5);
   std::vector<std::vector<int>> table;
   table.emplace_back(std::vector<int> {1,30,8});
   table.emplace_back(std::vector<int> {2,32,9});
   table.emplace_back(std::vector<int> {4,32,9});
   table.emplace_back(std::vector<int> {2,31,9});
   table.emplace_back(std::vector<int> {4,36,9});
   table.emplace_back(std::vector<int> {5,36,9});
   //cout << "TABLE:" << table.size() << "\n";
   
   TRYFAIL {
      cp->post(x[1] >= 36);
      cp->post(Factory::table(x, table)); 
      cp->post(x[0] != 5);
      cp->post(x[0] != 4);
      DFSearch search(cp,[=]() {
         auto xi = selectMin(x,
                             [](const auto& x) { return x->size() > 1;},
                             [](const auto& x) { return x->size();});
         if (xi) {
            int c = xi->min();                    
            return  [=] { cp->post(xi == c);}
               | [=] { cp->post(xi != c);};
         } else return Branches({});
      });    
      search.onSolution([&x]() {
         std::cout << "sol = " << x << std::endl;
      }); 
      auto stat = search.solve();
      std::cout << stat << std::endl;
   } ONFAIL {
      std::cout << "Something off infeasible from the start...\n";
   } ENDFAIL;
   cp.dealloc();
   return 0;
}
