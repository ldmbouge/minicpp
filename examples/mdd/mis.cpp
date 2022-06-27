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
   auto start = RuntimeMonitor::cputime();
   CPSolver::Ptr cp  = Factory::makeSolver();
   auto x = Factory::intVarArray(cp, 5, 0, 1);
   auto z = Factory::makeIntVar(cp,12,1000);
   auto mdd = Factory::makeMDDRelax(cp,2);
   Factory::sumMDD(mdd->getSpec(),x,{5,4,2,6,8},0,10000);
   Factory::sumMDD(mdd->getSpec(),{x[0],x[1]},{1,1},0, 1);
   Factory::sumMDD(mdd->getSpec(),{x[0],x[4]},{1,1},0, 1);
   Factory::sumMDD(mdd->getSpec(),{x[1],x[2]},{1,1},0, 1);
   Factory::sumMDD(mdd->getSpec(),{x[1],x[3]},{1,1},0, 1);
   Factory::sumMDD(mdd->getSpec(),{x[2],x[3]},{1,1},0, 1);
   Factory::sumMDD(mdd->getSpec(),{x[3],x[4]},{1,1},0, 1);
   cp->post(mdd);
   //auto obj = Factory::maximize(z);
   auto end = RuntimeMonitor::cputime();
   mdd->saveGraph();
   std::cout << "VARS: " << x << std::endl;
   std::cout << "Time : " << RuntimeMonitor::milli(start,end) << std::endl;
   
   DFSearch search(cp,[=]() {
      auto xk = selectMin(x,
                         [](const auto& xi) { return xi->size() > 1;},
                         [](const auto& xi) { return xi->size();});
      
      if (xk) {
         //mddAppliance->saveGraph();
         
         int c = xk->max();         
         return  [=] {
            std::cout << "choice  <" << xk << " == " << c << ">" << std::endl;
            cp->post(xk == c);
            //mdd->saveGraph();
            //std::cout << "VARS: " << x << std::endl;
         }
            | [=] {
               std::cout << "choice  <" << xk << " != " << c << ">" << std::endl;
               cp->post(xk != c);
               //mdd->saveGraph();                  
               //std::cout << "VARS: " << x << std::endl;
            };
      } else return Branches({});
   });
   
   search.onSolution([&x,&z]() {
      std::cout << "Assignment:" << x << "\t OBJ:" << z << std::endl;
   });        
   auto stat = search.solve();
   cout << stat << endl;
   cp.dealloc();
   return 0;
}
