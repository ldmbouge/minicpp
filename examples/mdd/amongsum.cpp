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
#include "mdd.hpp"
#include "RuntimeMonitor.hpp"

int main(int argc,char* argv[])
{
   int useSearch = 0;
    using namespace std;
    using namespace Factory;
    CPSolver::Ptr cp  = Factory::makeSolver();
   auto v = Factory::intVarArray(cp, 5, 1, 3);
   std::set<int> values_1 = {2};
   std::set<int> values_2 = {3};
   long start = RuntimeMonitor::cputime();
   MDDSpec state;
   Factory::amongMDD(state,v, 2, 2, values_1);
   Factory::amongMDD(state,v, 2, 2, values_2);
   auto mdd = new MDD(cp, v, false);
   mdd->setSpec(state);
   mdd->post();
   auto sv = Factory::intVarArray(cp,2);
   sv[0] = v[0];
   sv[1] = v[1];
   cp->post(sum(sv) <= v[4]);
   long end = RuntimeMonitor::cputime();
   std::cout << "Time : " << (end-start) << std::endl;
   mdd->saveGraph();

   if(useSearch){
      DFSearch search(cp,[=]() {
          auto x = selectMin(v,
                             [](const auto& x) { return x->size() > 1;},
                             [](const auto& x) { return x->size();});
          
          if (x) {
              //mddAppliance->saveGraph();

              int c = x->min();

              return  [=] {
                  cp->post(x == c);}
              | [=] {
                  cp->post(x != c);};
          } else return Branches({});
      });
      
      search.onSolution([&v, mdd]() {
          std::cout << "Assignment:" << std::endl;
         std::cout << v << std::endl;
      });
      
      
      auto stat = search.solve();
      end = RuntimeMonitor::cputime();
      std::cout << "Time : " << (end-start) << std::endl;
      cout << stat << endl;
      
   }
    cp.dealloc();
    return 0;
}
