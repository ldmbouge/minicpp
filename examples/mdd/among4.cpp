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
   int useSearch = 1;
    using namespace std;
    using namespace Factory;
    CPSolver::Ptr cp  = Factory::makeSolver();

    auto v = Factory::intVarArray(cp, 200, 1, 9);
   long start = RuntimeMonitor::cputime();
   auto mdd = new MDD(cp);
   Factory::amongMDD(mdd->getSpec(),v, 2, 5, {2});
   Factory::amongMDD(mdd->getSpec(),v, 2, 5, {3});
   Factory::amongMDD(mdd->getSpec(),v, 3, 5, {4});
   Factory::amongMDD(mdd->getSpec(),v, 3, 5, {5});

   cp->post(mdd);

   std::cout << "MDD Usage: " << mdd->usage() << std::endl;
   long end = RuntimeMonitor::cputime();
//   mdd->saveGraph();
   std::cout << "Time : " << (end-start) << std::endl;

   if(useSearch){
      DFSearch search(cp,[=]() {
          auto x = selectMin(v,
                             [](const auto& x) { return x->size() > 1;},
                             [](const auto& x) { return x->size();});
          
          if (x) {
             //mdd->saveGraph();

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
      
      
       auto stat = search.solve([](const SearchStatistics& stats) {
             return stats.numberOfSolutions() > 0;
         });
      cout << stat << endl;
   }
    cp.dealloc();
    return 0;
}
