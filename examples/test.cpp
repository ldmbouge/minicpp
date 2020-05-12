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
 */

#include <iostream>
#include <iomanip>
#include "solver.hpp"
#include "trailable.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"
#include <vector>

int main(int argc,char* argv[])
{
    using namespace std;
    using namespace Factory;
    const int n = 2;
    CPSolver::Ptr cp  = Factory::makeSolver();

   //  auto q = Factory::intVarArray(cp,n,1,n);
   //  cp->post(Factory::allDifferentAC(q));
   //  cp->post(q[0] != 2);
   auto w = Factory::makeIntVar(cp,{1,2});
   auto x = Factory::makeIntVar(cp,{1,2});
   auto y = Factory::makeIntVar(cp,{1,3});
   auto z = Factory::makeIntVar(cp,{2,4,5});

   std::vector<var<int>::Ptr> q;
   q.push_back(w);
   q.push_back(x);
   q.push_back(y);
   q.push_back(z);

    cp->post(Factory::allDifferentAC(q));
    std::cout << "done posting" << std::endl;
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
                         cout << "sol = " << q << endl;
                      });

    auto stat = search.solve();
    cout << stat << endl;
    
    cp.dealloc();
    return 0;
}
