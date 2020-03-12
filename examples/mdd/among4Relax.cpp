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

#include "solver.hpp"
#include "trailable.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"
#include "mdd.hpp"
#include "RuntimeMonitor.hpp"
#include "mddrelax.hpp"

#include <iostream>
#include <iomanip>

int main(int argc,char* argv[])
{
   int useSearch = 1;
   using namespace Factory;
   int width = (argc >= 2 && strncmp(argv[1],"-w",2)==0) ? atoi(argv[1]+2) : 16;
      
   CPSolver::Ptr cp  = Factory::makeSolver();

   auto v = Factory::intVarArray(cp, 50, 1, 9);
   auto start = RuntimeMonitor::now();
   auto mdd = new MDDRelax(cp,width);
   //auto mdd = new MDD(cp);
   Factory::amongMDD(mdd->getSpec(),v, 2, 5, {2});
   Factory::amongMDD(mdd->getSpec(),v, 2, 5, {3});
   Factory::amongMDD(mdd->getSpec(),v, 3, 5, {4});
   Factory::amongMDD(mdd->getSpec(),v, 3, 5, {5});

   cp->post(mdd);

   std::cout << "MDD Usage: " << mdd->usage() << std::endl;
   auto dur = RuntimeMonitor::elapsedSince(start);
   //   mdd->saveGraph();
   std::cout << "Time : " << dur << std::endl;

   if(useSearch){
      DFSearch search(cp,[=]() {
         unsigned i;
         for(i=0u;i< v.size();i++)
            if (v[i]->size() > 1)
               break;
         auto x = v[i];
/*
                            auto x = selectMin(v,
                                               [](const auto& x) { return x->size() > 1;},
                                               [](const auto& x) { return x->size();});
          */
         if (x) {
            int c = x->min();
            return  [=] {
                       //std::cout << "branch(" << i << ") ==" << c << std::endl;
                       cp->post(x == c);
                       // mdd->debugGraph();
                    }
               | [=] {
                    cp->post(x != c);
                    // std::cout << "branch!=" << std::endl;
                    // mdd->debugGraph();
                 };
         } else return Branches({});
                         });
      
      search.onSolution([&v]() {
                           std::cout << "Assignment:" << std::endl;
                           std::cout << v << std::endl;
                        });
      
      std::cout << "starting..." << std::endl;
      auto ss = RuntimeMonitor::now();
      auto stat = search.solve([](const SearchStatistics& stats) {
                                  return stats.numberOfSolutions() > 0;
                               });
      std::cout << stat << std::endl;
      auto atEndS = RuntimeMonitor::elapsedSince(ss);
      auto atEnd = RuntimeMonitor::elapsedSince(start);
      std::cout << "total:" << atEnd << "\t search:" << atEndS << std::endl;
   }
   cp.dealloc();
   return 0;
}

