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
   using namespace std;
   using namespace Factory;
   CPSolver::Ptr cp  = Factory::makeSolver();
   int min = 1; int max = 10;
   auto v = Factory::intVarArray(cp,20, min, max);
   auto x0  = Factory::intVarArray(cp,15,[&](int i) { return v[i];});
   auto x1  = Factory::intVarArray(cp,15,[&](int i) { return v[i+5];});
   auto mdd = Factory::makeMDDRelax(cp,2);
   std::map<int,int> boundsC1 = {
      {1,3},{2,3},{3,3},
      {4,3},{5,3},{6,3},
      {7,3},{8,3},{9,10},
      {10,0}
   };
   std::map<int,int> boundsC2 = {
      {1,0},{2,1},{3,1},
      {4,0},{5,0},{6,2},
      {7,1},{8,2},{9,3},
      {10,10}
   };
   TRYFAIL {
      mdd->post(atMostMDD2(mdd,x0,boundsC1));   
      mdd->post(atMostMDD2(mdd,x1,boundsC2));
      cp->post(mdd);
   } ONFAIL {
      std::cout << "Infeasible model...\n";
      return 1;
   }
   ENDFAIL;
      
   DFSearch search(cp,[=]() {
      return indomain_min(cp,selectFirst(v,[](const auto& x) { return x->size() > 1;}));
   });
      
   search.onSolution([&v]() {
      std::cout << "Assignment:" << v << "\n";
   });      
      
   auto stat = search.solve([](const SearchStatistics& stats) {
      return stats.numberOfSolutions() > 0;
   });

   cout << stat << endl;
   cp.dealloc();
   return 0;
}
