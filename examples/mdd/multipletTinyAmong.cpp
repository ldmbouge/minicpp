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

//temporary we should get variable from solver
auto merge(CPSolver::Ptr cp, const Factory::Veci& v1,const Factory::Veci& v2)
{
   int sz = (int)(v1.size() + v2.size());
   auto r = Factory::intVarArray(cp, sz);
   int  i  =  0;
   for(auto e : v1)
      r[i++] = e;
   for(auto e : v2)
      r[i++] = e;
   return r;
}

int main(int argc,char* argv[])
{
   int useSearch = 1;
   using namespace std;
   using namespace Factory;
   CPSolver::Ptr cp  = Factory::makeSolver();

   auto v = Factory::intVarArray(cp, 2, 1, 3);
   auto v2 = Factory::intVarArray(cp, 2, 1, 3);
   std::set<int> values_1 = {2};
   std::set<int> values_2 = {3};
   long start = RuntimeMonitor::cputime();
   MDDSpec state;
   Factory::amongMDD(state,v, 2, 2, values_1);
   Factory::amongMDD(state,v2, 1, 1, values_1);
   auto mdd = new MDD(cp);
   mdd->setState(state);

   cp->post(mdd);
   
   long end = RuntimeMonitor::cputime();
   mdd->saveGraph();
   auto vars = merge(cp,v,v2);
   std::cout << "VARS: " << vars << std::endl;
   std::cout << "Time : " << (end-start) << std::endl;
   
   if(useSearch){
      DFSearch search(cp,[=]() {
          auto x = selectMin(vars,
                             [](const auto& x) { return ((var<int>::Ptr)x)->size() > 1;},
                             [](const auto& x) { return ((var<int>::Ptr)x)->size();});
          
          if (x) {
              int c = x->min();
              return  [=] {
                         std::cout << "choice  <" << x << " == " << c << ">" << std::endl;
                         cp->post(x == c);
//                         mdd->saveGraph();
                         std::cout << "VARS: " << v << std::endl;
                      }
                 | [=] {
                      std::cout << "choice  <" << x << " != " << c << ">" << std::endl;
                      cp->post(x != c);
//                      mdd->saveGraph();
                      std::cout << "VARS: " << v << std::endl;
                   };
          } else return Branches({});
      });
      
      search.onSolution([&vars, mdd]() {
         std::cout << "Assignment:" << std::endl;
         std::cout << "v:" << vars  << std::endl;
      });
      
      
       auto stat = search.solve([](const SearchStatistics& stats) {
             return stats.numberOfSolutions() > 0;
         });
      cout << stat << endl;
   }
    cp.dealloc();
    return 0;
}
