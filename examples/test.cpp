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
   const int n = 3;
   // CPSolver::Ptr cp  = Factory::makeSolver();
   ExpSolver::Ptr cp  = Factory::makeExpSolver();

   auto q = Factory::intVarArray(cp,n,1,n);

   // auto l0 = Literal(q[0], NEQ, 1, nullptr, -1);
   // auto l1 = Literal(q[1], NEQ, 2, nullptr, -1);
   // auto lhs = LitHashSet();
   // lhs.insert(l0);
   // lhs.insert(l1);

   //  auto exp = Explainer(cp);
   //  exp.inject();
   cp->post(Factory::allDifferentAC(q));
   //  cp->post(q[0] != 2);
   //  cp->post(q[1] != 4);
   //  cp->post(q[2] != 1);



   // auto w = Factory::makeIntVar(cp,{1,2});
   // auto x = Factory::makeIntVar(cp,{1,2});
   // auto y = Factory::makeIntVar(cp,{1,3});
   // auto z = Factory::makeIntVar(cp,{2,4,5});
   // auto x = -10 * w;
   // std::vector<var<int>::Ptr> q;
   // q.push_back(w);
   // q.push_back(x);
   // q.push_back(y);
   // q.push_back(z);

   // auto w = IntVarImpl(cp,1,10);
   // std::cout << "testing intvar iterator" << std::endl;
   // std::cout << "w = {1,2}" << std::endl;
   // for (auto val : *w) {
   //    std::cout << val << std::endl;
   // }
   // std::cout << "x = {1,2}" << std::endl;
   // for (auto val : *x) {
   //    std::cout << val << std::endl;
   // }
   // std::cout << "y = {1,3}" << std::endl;
   // for (auto val : *y) {
   //    std::cout << val << std::endl;
   // }
   // std::cout << "z = {2,4,5}" << std::endl;
   // for (auto val : *z) {
   //    std::cout << val << std::endl;
   // }

    cp->post(Factory::allDifferentAC(q));
    std::cout << "done posting" << std::endl;
    ExpDFSearch search(cp,[=]() {
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
