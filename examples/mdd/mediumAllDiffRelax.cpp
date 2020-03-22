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

#define SZ_VAR 10
#define SZ_VAL 10


using namespace std;
using namespace Factory;

void solveModel(CPSolver::Ptr cp)
{
   auto vx = cp->intVars();
   DFSearch search(cp,[=]() {
      // auto x = selectMin(vx,
      //                    [](const auto& x) { return x->size() > 1;},
      //                    [](const auto& x) { return x->size();});
      unsigned i;      
      for(i=0u;i< vx.size();i++)
         if (vx[i]->size() > 1)
            break;
      auto x = i < vx.size() ? vx[i] : nullptr;
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
   int width = (argc >= 2 && strncmp(argv[1],"-w",2)==0) ? atoi(argv[1]+2) : 2;
   CPSolver::Ptr cp  = Factory::makeSolver();
   auto v = Factory::intVarArray(cp, SZ_VAR, 1, SZ_VAL);
   auto start = RuntimeMonitor::cputime();
   auto mdd = new MDDRelax(cp,width);
   Factory::allDiffMDD(mdd->getSpec(),v);
   cp->post(mdd);
   mdd->saveGraph();
   auto end = RuntimeMonitor::cputime();
   MDDStats stats(mdd);
   std::cout << "MDD Usage:" << mdd->usage() << std::endl;
   std::cout << "Time : " << RuntimeMonitor::milli(start,end) << std::endl;
   std::cout << stats << std::endl;
   solveModel(cp);
   cp.dealloc();
   return 0;
}
