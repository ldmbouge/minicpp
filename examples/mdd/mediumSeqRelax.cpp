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


#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <regex>
#include <fstream>      // std::ifstream
#include <iomanip>
#include <iostream>
#include "solver.hpp"
#include "trailable.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"
#include "mddrelax.hpp"
#include "mddConstraints.hpp"

#include "RuntimeMonitor.hpp"
#include "matrix.hpp"

#define SZ_VAR 20
#define SZ_VAL 10
#define LB 2
#define UB 2
#define LEN 5

using namespace std;
using namespace Factory;

template <class Vec>
void solveModel(CPSolver::Ptr cp,Vec& vx)
{
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
      std::cout << "Assignment: " << vx << std::endl;
   });

   auto stat = search.solve([](const SearchStatistics& stats) {
                               return stats.numberOfSolutions() > 0;
                            });
   std::cout << stat << std::endl;
}


int main(int argc,char* argv[])
{
   int width = (argc >= 2 && strncmp(argv[1],"-w",2)==0) ? atoi(argv[1]+2) : 16;
   CPSolver::Ptr cp  = Factory::makeSolver();
   auto v = Factory::intVarArray(cp, SZ_VAR, 1, SZ_VAL);
   auto start = RuntimeMonitor::cputime();
   auto mdd = new MDDRelax(cp,width);
   mdd->post(Factory::seqMDD(mdd,v,LEN,LB,UB,{2,4,5,6}));
   cp->post(mdd);
   auto end = RuntimeMonitor::cputime();
   MDDStats stats(mdd);
   std::cout << "MDD Usage:" << mdd->usage() << std::endl;
   std::cout << "Time : " << RuntimeMonitor::milli(start,end) << std::endl;
   std::cout << stats << std::endl;
   solveModel(cp,v);
   cp.dealloc();
   return 0;
}
