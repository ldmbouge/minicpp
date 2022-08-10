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
 * Copyright (c)  2018. by Laurent Michel, Willem Jan van Hoeve, Pierre Schaus, Pascal Van Hentenryck
 */

#include <iostream>
#include <iomanip>
#include <limits>
#include <string.h>
#include "solver.hpp"
#include "trailable.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"
#include "RuntimeMonitor.hpp"
#include "table.hpp"
#include "range.hpp"

using namespace std;
int main(int argc,char* argv[]) {
   using namespace Factory;
   int N     = (argc >= 2 && strncmp(argv[1],"-n",2)==0) ? atoi(argv[1]+2) : 8;

   CPSolver::Ptr cp  = Factory::makeSolver();
   auto x = Factory::intVarArray(cp, N, 0, N-1);
   auto y = Factory::intVarArray(cp, N-1, 0, N-1);   
   for (int i=0; i<N-1; i++) 
      cp->post(y[i] != 0);

   cp->post(Factory::allDifferentAC(x));
   cp->post(Factory::allDifferentAC(y));
   for (int i=0; i<N-1; i++) 
      cp->post(Factory::equalAbsDiff(y[i], x[i+1], x[i]));   

   DFSearch search(cp,[=]() {
      return indomain_min(cp,selectFirst(x,[](const auto& x) { return x->size() > 1;}));
   });
   SearchStatistics stat = search.solve();
   std::cout << stat << "\n";
   return 0;
}
