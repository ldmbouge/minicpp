
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
   int useSearch = 1;
   int debug = 0;
    using namespace std;
    using namespace Factory;
    CPSolver::Ptr cp  = Factory::makeSolver();

    auto v = Factory::intVarArray(cp,5, 1, 3);
   auto start = RuntimeMonitor::cputime();
   auto mdd = Factory::makeMDDRelax(cp,4);
   mdd->post(amongMDD(mdd,v,2,2,{3}));
   mdd->post(seqMDD(mdd,v,3,2,2,{2,3}));
   mdd->post(gccMDD(mdd,v,{{2,1},{1,2}, {3, 2}}));

   cp->post(mdd);
   
   auto end = RuntimeMonitor::cputime();
   mdd->saveGraph();
    std::cout << "VARS: " << v << std::endl;
   std::cout << "Time : " << RuntimeMonitor::milli(start,end) << std::endl;
   
   if(useSearch){
      DFSearch search(cp,[=]() {
          auto x = selectMin(v,
                             [](const auto& x) { return x->size() > 1;},
                             [](const auto& x) { return x->size();});
          
          if (x) {
              //mddAppliance->saveGraph();
              int c = x->min();
              return  [=] {
                 if(debug)
                    std::cout << "choice  <" << x << " == " << c << ">" << std::endl;
                 cp->post(x == c);
                 if (debug)
                    std::cout << "VARS: " << v << std::endl;
                      }
                 | [=] {
                    if(debug)
                      std::cout << "choice  <" << x << " != " << c << ">" << std::endl;
                    cp->post(x != c);
                    if(debug)
                      std::cout << "VARS: " << v << std::endl;
                   };
          } else return Branches({});
      });
      
      search.onSolution([&v]() {
          std::cout << "Assignment:" << std::endl;
         std::cout << v << std::endl;
      });
      
      
       auto stat = search.solve([](const SearchStatistics& stats) {
             return stats.numberOfSolutions() > 0;
         });
      cout << stat << endl;
   }
    cp.dealloc();
    return 0;
}
