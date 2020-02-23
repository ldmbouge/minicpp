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
#include "RuntimeMonitor.hpp"

#define SZ_VAR 10
#define SZ_VAL 4


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

std::map<int,int> buildBounds(int nbVars, int min, int max)
{
   std::map<int,int> bounds;
   int n =  nbVars / (max - min + 1);
   int r = nbVars % (max - min + 1);
   for(int i = min; i <= max; i++) {
      int k = n + (r-- > 0 ? 1 : 0);      
      bounds.insert(std::pair<int,int>(i,k));
   }
   return bounds;
}

int main(int argc,char* argv[])
{
   int width = (argc >= 2 && strncmp(argv[1],"-w",2)==0) ? atoi(argv[1]+2) : 16;   
   CPSolver::Ptr cp  = Factory::makeSolver();
   auto v = Factory::intVarArray(cp, SZ_VAR, 1, SZ_VAL);
   auto start = RuntimeMonitor::cputime();
   auto mdd = new MDDRelax(cp,4);
   //auto bounds = buildBounds(SZ_VAR, 1, SZ_VAL);
   std::map<int,int> bounds = {{1,1},{2,2},{3,3}};
   for(auto& b : bounds) {
      std::cout << b.first << " : " << b.second << std::endl;
   }
   Factory::gccMDD(mdd->getSpec(),v,bounds);
   cp->post(mdd);
   auto end = RuntimeMonitor::cputime();
   MDDStats stats(mdd);
   std::cout << "MDD Usage:" << mdd->usage() << std::endl;
   std::cout << "Time : " << RuntimeMonitor::milli(start,end) << std::endl;
   std::cout << stats << std::endl;
   solveModel(cp);
   cp.dealloc();
   return 0;
}
