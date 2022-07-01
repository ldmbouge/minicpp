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
#include "mddrelax.hpp"
#include "mddConstraints.hpp"
#include "RuntimeMonitor.hpp"

int main(int argc,char* argv[])
{
   using namespace std;
   using namespace Factory;
   CPSolver::Ptr cp  = Factory::makeSolver();
   auto x = Factory::intVarArray(cp, 5, 0, 1);
   auto z = Factory::makeIntVar(cp,0,10000);
   auto mdd = Factory::makeMDDRelax(cp,4);
   mdd->post(sum(x,{5,4,2,6,8},z));
   mdd->post(sum({x[0],x[1]},0, 1));
   mdd->post(sum({x[0],x[4]},0, 1));
   mdd->post(sum({x[1],x[2]},0, 1));
   mdd->post(sum({x[1],x[3]},0, 1));
   mdd->post(sum({x[2],x[3]},0, 1));
   mdd->post(sum({x[3],x[4]},0, 1));
   cp->post(mdd);
   auto obj = Factory::maximize(z);
   mdd->saveGraph();
   std::cout << "VARS: " << x << "\t Z=" << z <<  "\n";
   
   DFSearch search(cp,[=]() {
      auto xk = selectMin(x,
                          [](const auto& xi) { return xi->size() > 1;},
                          [](const auto& xi) { return xi->size();});
      if (xk) {        
       int c = xk->max();         
       return  [=] {
         std::cout << "choice  <" << xk << " == " << c << ">\n";
         cp->post(xk == c);
       }
         | [=] {
           std::cout << "choice  <" << xk << " != " << c << ">\n";
           cp->post(xk != c);
         };
     } else return Branches({});
   });
   
   search.onSolution([&x,&z]() {
      std::cout << "Assignment:" << x << "\t OBJ:" << z << "\n";
   });        
   auto stat = search.optimize(obj);
   cout << stat << endl;
   cp.dealloc();
   return 0;
}
