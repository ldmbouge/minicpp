
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
#include "mddrelax.hpp"

int main(int argc,char* argv[])
{
   int useSearch = 1;
   using namespace std;
   using namespace Factory;
   int width = (argc >= 2 && strncmp(argv[1],"-w",2)==0) ? atoi(argv[1]+2) : 2;

   CPSolver::Ptr cp  = Factory::makeSolver();
   const int nb = 5;
   auto v = Factory::intVarArray(cp, nb, 0, nb-1);
   auto start = RuntimeMonitor::cputime();
   auto mdd = new MDDRelax(cp,width);
   Factory::allDiffMDD(mdd->getSpec(),v);

   cp->post(mdd);
   
   std::cout << "MDD Usage: " << mdd->usage() << std::endl;
   auto end = RuntimeMonitor::cputime();
   mdd->saveGraph();
   std::cout << "VARS: " << v << std::endl;
   std::cout << "Time : " << RuntimeMonitor::milli(start,end) << std::endl;
   
   if(useSearch){
      DFSearch search(cp,[=]() {
                            unsigned i = 0u;
                            for(i=0u;i < v.size();i++)
                               if (v[i]->size()> 1) break;
                            auto x = i< v.size() ? v[i] : nullptr;

                            // auto x = selectMin(v,
                            //                    [](const auto& x) { return x->size() > 1;},
                            //                    [](const auto& x) { return x->size();});
          
                            if (x) {
                               //mddAppliance->saveGraph();

                               int c = x->min();

                               return  [=] {
                                          std::cout << "choice  <" << x << " == " << c << ">" << std::endl;
                                          cp->post(x == c);
                                          //                         mdd->saveGraph();
                                          //std::cout << "VARS: " << v << std::endl;
                                       }
                                  | [=] {
                                       std::cout << "choice  <" << x << " != " << c << ">" << std::endl;
                                       cp->post(x != c);
                                       //                      mdd->saveGraph();
                                       //std::cout << "VARS: " << v << std::endl;
                                    };
                            } else return Branches({});
                         });
      
      search.onSolution([&v]() {
                           std::cout << "Assignment:"  << v << std::endl;
                        });
      
      
      auto stat = search.solve([](const SearchStatistics& stats) {
                                  return stats.numberOfSolutions() > 0;
                               });
      cout << stat << endl;
   }
   cp.dealloc();
   return 0;
}
