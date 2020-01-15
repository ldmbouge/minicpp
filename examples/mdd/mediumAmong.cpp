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

#define SZ_VAR 20
#define SZ_VAL 20
#define LB 2
#define UB 2


using namespace std;
using namespace Factory;

void solveModel(CPSolver::Ptr cp)
{
   auto vx = cp->intVars();
   DFSearch search(cp,[=]() {
      auto x = selectMin(vx,
                         [](const auto& x) { return x->size() > 1;},
                         [](const auto& x) { return x->size();});
      if (x) {
         int c = x->min();
         
         return  [=] {
            cp->post(x == c);}
         | [=] {
            cp->post(x != c);};
      } else return Branches({});
   });
   
   search.onSolution([&vx]() {
      std::cout << "Assignment:" << std::endl;
      std::cout << vx << std::endl;
   });
   
   auto stat = search.solve([](const SearchStatistics& stats) {
      return stats.numberOfSolutions() > 0;
   });
   std::cout << stat << std::endl;
}


int main(int argc,char* argv[])
{
   CPSolver::Ptr cp  = Factory::makeSolver();
   auto v = Factory::intVarArray(cp, SZ_VAR, 1, SZ_VAL);
   long start = RuntimeMonitor::cputime();
   auto mdd = new MDD(cp);
   Factory::amongMDD(mdd->getSpec(),v,LB,UB,{2,4,5,6,8});
   cp->post(mdd);
   long end = RuntimeMonitor::cputime();
   MDDStats stats(mdd);
   std::cout << "MDD Usage:" << mdd->usage() << std::endl;
   std::cout << "Time : " << (end-start) << std::endl;
   std::cout << stats << std::endl;
   solveModel(cp);
   cp.dealloc();
   return 0;
}
