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
#include <functional>
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

   ExpSolver::Ptr cp  = Factory::makeExpSolver();

   auto q = Factory::boolVarArray(cp,14);

   cp->post(q[1] != q[9]);   // nX2
   cp->post(q[2] != q[10]);  // nX3
   cp->post(q[3] != q[11]);  // nX4
   cp->post(q[4] != q[12]);  // nX5
   cp->post(q[5] != q[13]);  // nX6

   auto c1 = std::vector<var<bool>::Ptr>({q[0],q[1]});
   auto c2 = std::vector<var<bool>::Ptr>({q[0],q[2],q[6]});
   auto c3 = std::vector<var<bool>::Ptr>({q[9],q[10],q[3]});
   auto c4 = std::vector<var<bool>::Ptr>({q[11],q[4],q[7]});
   auto c5 = std::vector<var<bool>::Ptr>({q[11],q[5],q[8]});
   auto c6 = std::vector<var<bool>::Ptr>({q[12],q[13]});

   cp->post(Factory::clause(c1));
   cp->post(Factory::clause(c2));
   cp->post(Factory::clause(c3));
   cp->post(Factory::clause(c4));
   cp->post(Factory::clause(c5));
   cp->post(Factory::clause(c6));

   // x7 == 0 @ 1
   std::function<void(void)> b1 = [=]() {
      cp->post(q[6] == false);
   };

   // x8 == 0 @ 2
   std::function<void(void)> b2 = [=]() {
      cp->post(q[7] == false);
   };

   // x9 == 0 @ 3
   std::function<void(void)> b3 = [=]() {
      cp->post(q[8] == false);
   };

   // x1 == 0 @ 4
   std::function<void(void)> b4 = [=]() {
      cp->post(q[0] == false);
   };

   std::vector<std::function<void(void)>> choices;
   choices.push_back(b1);
   choices.push_back(b2);
   choices.push_back(b3);
   choices.push_back(b4);

   ExpTestSearch search(cp);

   // ExpDFSearch search(cp,[=]() {
   //                      auto x = selectMin(q,
   //                                        [](const auto& x) { return x->size() > 1;},
   //                                        [](const auto& x) { return x->size();});
   //                      if (x) {
   //                         return  [=] { cp->post(x == true);}
   //                            | [=] { cp->post(x == false);};
   //                      } else return Branches({});
   //                   });    

   //  search.onSolution([&q]() {
   //                       cout << "sol = " << q << endl;
   //                    });

   auto stat = search.solve(choices);
   cout << stat << endl;
    
   cp.dealloc();
   return 0;
}
