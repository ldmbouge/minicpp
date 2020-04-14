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

#define SZ_VAR 4
#define SZ_VAL 3


using namespace std;
using namespace Factory;

void solveModel(CPSolver::Ptr cp, const Veci& vx)
{
   DFSearch search(cp,[=]() {

       // auto x = selectMin(vx,
       // 			  [](const auto& x) { return x->size() > 1;},
       // 			  [](const auto& x) { return x->size();});
       
      unsigned i;
      for(i=0u;i< vx.size();i++)
	if (vx[i]->size() > 1)
	  break;
      
      auto x = i < vx.size() ? vx[i] : nullptr;

      if (x) {
         int c = x->min();
         
         return  [=] {
	   // std::cout << "choice  <" << x << " == " << c << ">" << std::endl;
	   cp->post(x == c);
	   // std::cout << "VARS: " << vx << std::endl;
	 }
         | [=] {
	   // std::cout << "choice  <" << x << " != " << c << ">" << std::endl;
	   cp->post(x != c);
	   // std::cout << "VARS: " << vx << std::endl;
	 };
      } else return Branches({});
   });
   
   search.onSolution([&vx]() {
      std::cout << "Assignment:" << std::endl;
      std::cout << vx << std::endl;
   });
   
   auto stat = search.solve([](const SearchStatistics& stats) {
      return stats.numberOfSolutions() > 100000;
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


void addGCC(CPSolver::Ptr cp, const Veci& vars, const std::map<int,int>& lb, const std::map<int,int>& ub ) {

  // meet demand: count occurrence of configuration via a Boolean variable
  std::cout << "use standard Boolean counters to model the GCC: ";

  std::map<int,int>::const_iterator it1, it2;
  for (it1=lb.begin(), it2=ub.begin(); it1!=lb.end(); ++it1, ++it2) {
    assert(it1->first == it2->first);
    auto boolVar = Factory::boolVarArray(cp,(int) vars.size());
    std::set<int> S;
    S.insert(it1->first);
    for (int i=0; i<(int) vars.size(); i++) {
      cp->post(isMember(boolVar[i], vars[i], S));
    }
    cp->post(sum(boolVar) >= it1->second);
    cp->post(sum(boolVar) <= it2->second);
  }
  std::cout << "done." << std::endl;
}


int main(int argc,char* argv[])
{
   int width = (argc >= 2 && strncmp(argv[1],"-w",2)==0) ? atoi(argv[1]+2) : 1;   
   int mode = (argc >= 3 && strncmp(argv[2],"-m",2)==0) ? atoi(argv[2]+2) : 0;   

   std::cout << "width = " << width << std::endl;
   std::cout << "mode = " << mode << std::endl;

   CPSolver::Ptr cp  = Factory::makeSolver();

   auto mdd = new MDDRelax(cp,width);


   auto v = Factory::intVarArray(cp, SZ_VAR, 1, SZ_VAL);
   //auto bounds = buildBounds(SZ_VAR, 1, SZ_VAL);
   std::map<int,int> boundsLB = {{1,1},{2,1},{3,2}};
   std::map<int,int> boundsUB = {{1,2},{2,1},{3,2}};

   auto start = RuntimeMonitor::cputime();
   
   std::cout << "lower and upper bounds:" << std::endl;
     
   for(auto& b : boundsLB) {
      std::cout << b.first << " : " << b.second << std::endl;
   }
   for(auto& b : boundsUB) {
      std::cout << b.first << " : " << b.second << std::endl;
   }

   if (mode == 0) {
     std::cout << "use Boolean domain encoding for GCC" << std::endl;
     addGCC(cp, v, boundsLB, boundsUB);
   }
   else if (mode == 1) {
     std::cout << "use gccMDD -- only with UB constraints!" << std::endl;
     Factory::gccMDD(mdd->getSpec(), v, boundsUB);
     cp->post(mdd);
     mdd->saveGraph();
   }
   else if (mode == 2) {
     std::cout << "use gccMDD2" << std::endl;
     Factory::gccMDD2(mdd->getSpec(), v, boundsLB, boundsUB);
     cp->post(mdd);
     mdd->saveGraph();
   }
   else {
     std::cout << "exit: mode should be 0 (Boolean domains), 1 (gccMDD), or 2 (gccMDD2)" << std::endl;
     exit(1);
   }

   auto end = RuntimeMonitor::cputime();
   if (mode != 0) {
     MDDStats stats(mdd);
     std::cout << "MDD Usage:" << mdd->usage() << std::endl;
     std::cout << "Time : " << RuntimeMonitor::milli(start,end) << std::endl;
     std::cout << stats << std::endl;
   }
   
   solveModel(cp, v);
   cp.dealloc();
   return 0;
}
