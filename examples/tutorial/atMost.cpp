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
#include "solver.hpp"
#include "trailable.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"
#include "mddrelax.hpp"
#include "mddConstraints.hpp"
#include "RuntimeMonitor.hpp"

namespace  Factory {
   MDDCstrDesc::Ptr atMostMDD(MDD::Ptr m,const Factory::Veci& vars,const std::map<int,int>& ub)
   {
      MDDSpec& spec = m->getSpec();
      auto [minFDom,minLDom] = domRange(vars);
      auto desc = spec.makeConstraintDescriptor(vars,"atMostMDD");

      std::map<int,MDDPInt::Ptr> pd;
      for(int i=minFDom; i <= minLDom;++i)
         pd[i] = spec.downIntState(desc,0,INT_MAX,MinFun);

      spec.arcExist(desc,[=](const auto& parent,const auto& child,auto x,int v) {
         return parent.down[pd.at(v)] < ub.at(v);
      });

      for(int i=minFDom; i <= minLDom;++i)
         spec.transitionDown(desc,pd[i],{pd[i]},{},[=](auto& out,const auto& parent,auto x, const auto& val) {
            out[pd.at(i)] = parent.down[pd.at(i)] + (val.isSingleton() ? val.contains(i) : 0);
         });

      return desc;
   }

   MDDCstrDesc::Ptr atMostMDD2(MDD::Ptr m,const Factory::Veci& vars,const std::map<int,int>& ub)
   {
      MDDSpec& spec = m->getSpec();
      auto udom = domRange(vars);
      const int minFDom = udom.first, minLDom = udom.second;
      ValueMap<int> values(udom.first, udom.second,0,ub);
      auto desc = spec.makeConstraintDescriptor(vars,"atMostMDD");

      std::map<int,MDDPInt::Ptr> pd;
      for(int i=minFDom; i <= minLDom;++i)
         pd[i] = spec.downIntState(desc,0,INT_MAX,MinFun);
      std::map<int,MDDPInt::Ptr> pu;
      for(int i=minFDom; i <= minLDom;++i)
         pu[i] = spec.upIntState(desc,0,INT_MAX,MinFun);

      spec.arcExist(desc,[=](const auto& parent,const auto& child,auto x,int v) {
         return parent.down[pd.at(v)] + child.up[pu.at(v)] < values[v]; 
      });

      for(int i=minFDom; i <= minLDom;++i)
         spec.transitionDown(desc,pd[i],{pd[i]},{},[=](auto& out,const auto& parent,auto,const auto& val) {
            out[pd.at(i)] = parent.down[pd.at(i)] + (val.isSingleton() ? val.contains(i) : 0);
         });

      for(int i=minFDom; i <= minLDom;++i)
         spec.transitionUp(desc,pu[i],{pu[i]},{},[=](auto& out,const auto& child,auto,const auto& val) {
            out[pu.at(i)] = child.up[pu.at(i)] + (val.isSingleton() ? val.contains(i) : 0);
         });
      return desc;
   }
}


int main(int argc,char* argv[])
{
   using namespace std;
   using namespace Factory;
   CPSolver::Ptr cp  = Factory::makeSolver();
   int min = 1; int max = 10;
   auto v = Factory::intVarArray(cp,20, min, max);
   auto x0  = Factory::intVarArray(cp,15,[&](int i) { return v[i];});
   auto x1  = Factory::intVarArray(cp,15,[&](int i) { return v[i+5];});
   auto mdd = Factory::makeMDDRelax(cp,8);
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
      mdd->post(atMostMDD(mdd,x0,boundsC1));   
      mdd->post(atMostMDD(mdd,x1,boundsC2));
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
