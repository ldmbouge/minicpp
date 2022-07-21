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

#include "mddConstraints.hpp"
#include "mddnode.hpp"
#include <limits.h>

namespace Factory {

   MDDCstrDesc::Ptr gccMDD(MDD::Ptr m,const Factory::Veci& vars,const std::map<int,int>& ub)
   {
      MDDSpec& spec = m->getSpec();
      int sz = (int) vars.size();
      auto udom = domRange(vars);
      const int dz = udom.second - udom.first + 1; // size of union of all domains
      const int minFDom = udom.first, minLDom = udom.second;
      const int maxFDom = dz + udom.first,maxLDom = dz + udom.second;
      ValueMap<int> values(udom.first, udom.second,0,ub);
      auto desc = spec.makeConstraintDescriptor(vars,"gccMDD");

      std::map<int,MDDPInt::Ptr> ps;
      for(int i=minFDom; i <= maxLDom;++i)
         ps[i] = spec.downIntState(desc,0,sz,i >= maxFDom ? MaxFun : MinFun);

      spec.arcExist(desc,[=](const auto& parent,const auto& child,auto x,int v) {
         return parent.down[ps.at(v)] < values[v];
      });

      for(int i=minFDom; i <= minLDom;++i)
         spec.transitionDown(desc,ps[i],{ps[i]},{},[=](auto& out,const auto& parent,auto x, const auto& val) {
            out[ps.at(i)] = parent.down[ps.at(i)] + (val.isSingleton() ? (val.singleton() == i) : 0);
         });

      for(int i=maxFDom; i <= maxLDom;++i)
         spec.transitionDown(desc,ps[i],{ps[i]},{},[=](auto& out,const auto& parent,auto x, const auto& val) {
            out[ps.at(i)] = parent.down[ps.at(i)] + (val.isSingleton() ? (val.singleton() == i - dz) : 0);
         });

      return desc;
   }

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
         return parent.down[pd.at(v)] + child.up[pu.at(v)] < values[v]; // we are given value v, use prefix/suffix counters and leave room (1) for this arc
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

   
   
   MDDCstrDesc::Ptr gccMDD2(MDD::Ptr m,const Factory::Veci& vars,const std::map<int,int>& lb,const std::map<int,int>& ub)
   {
      MDDSpec& spec = m->getSpec();
      int sz = (int) vars.size();
      auto udom = domRange(vars);
      int dz = udom.second - udom.first + 1;
      int minFDom = udom.first,    minLDom = udom.second;
      int maxFDom = dz+udom.first, maxLDom = dz+udom.second;
      ValueMap<int> valuesLB(udom.first, udom.second,0,lb);
      ValueMap<int> valuesUB(udom.first, udom.second,0,ub);
      auto desc = spec.makeConstraintDescriptor(vars,"gccMDD");
      std::map<int,MDDPInt::Ptr> dps,ups;
      for(int i=minFDom;i <= maxLDom;++i)
         dps[i] = spec.downIntState(desc,0,sz,i >=maxFDom ? MaxFun : MinFun);
      for(int i=minFDom;i <= maxLDom;++i)
         ups[i] = spec.upIntState(desc,0,sz,i >=maxFDom ? MaxFun : MinFun);

      spec.arcExist(desc,[=](const auto& parent,const auto& child,auto x,int v) {
         bool cond = true;
         const int minIdx = v;
         const int maxIdx = dz + v;
         // check LB and UB thresholds when value v is assigned:
         cond = cond && (parent.down[dps.at(minIdx)] + 1 + child.up[ups.at(minIdx)] <= valuesUB[v])
                     && (parent.down[dps.at(maxIdx)] + 1 + child.up[ups.at(maxIdx)] >= valuesLB[v]);
         // check LB and UB thresholds for other values, when they are not assigned:
         for (int i=minFDom; i<=minLDom; i++) {
            if (i==v) continue;
            if (!cond) break;
            cond = cond && (parent.down[dps.at(i)] + child.up[ups.at(i)]       <= valuesUB[i])
                        && (parent.down[dps.at(dz+i)] + child.up[ups.at(dz+i)] >= valuesLB[i]);
         }
         return cond;
      });
      
      spec.nodeExist([=](const auto& n) {
         // check global validity: can we still satisfy all lower bounds?
         int remainingLB=0;
         int fixedValues=0;
         for (int i=minFDom; i<=minLDom; i++) {
    	    remainingLB += std::max(0, valuesLB[i] - (n.down[dps.at(i)] + n.up[ups.at(i)]));
            fixedValues += n.down[dps.at(i)] + n.up[ups.at(i)];
         }
         return (fixedValues+remainingLB<=sz);
      });

      for(int i=minFDom;i <= minLDom;++i)
         spec.transitionDown(desc,dps[i],{dps[i]},{},[=](auto& out,const auto& parent,auto x,const auto& val) {
            out[dps.at(i)] = parent.down[dps.at(i)] + (val.isSingleton() && (val.singleton() == i)); 
         });
      
      for(int i=maxFDom;i <= maxLDom;++i) 
         spec.transitionDown(desc,dps[i],{dps[i]},{},[=](auto& out,const auto& parent,auto x,const auto& val) {
            out[dps.at(i)] = parent.down[dps.at(i)] + val.contains(i - dz);
         });
      
      for(int i=minFDom;i <= minLDom;++i)
         spec.transitionUp(desc,ups[i],{ups[i]},{},[=](auto& out,const auto& child,auto x,const auto& val) {
            out[ups.at(i)] = child.up[ups.at(i)] + (val.isSingleton() && (val.singleton() == i));
         });
      
      for(int i=maxFDom;i <= maxLDom;++i) 
         spec.transitionUp(desc,ups[i],{ups[i]},{},[=](auto& out,const auto& child,auto x,const auto& val) {
            out[ups.at(i)] = child.up[ups.at(i)] + val.contains(i - dz);
         });
      
      return desc;
   }
}
