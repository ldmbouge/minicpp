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

   void gccMDD(MDDSpec& spec,const Factory::Veci& vars,const std::map<int,int>& ub)
   {
      spec.append(vars);
      int sz = (int) vars.size();
      auto udom = domRange(vars);
      int dz = udom.second - udom.first + 1;
      int minFDom = 0, minLDom = dz-1;
      int maxFDom = dz,maxLDom = dz*2-1;
      int min = udom.first;
      ValueMap<int> values(udom.first, udom.second,0,ub);
      auto desc = spec.makeConstraintDescriptor(vars,"gccMDD");

      std::vector<int> ps = spec.addStates(desc,minFDom, maxLDom,sz,[] (int i) -> int { return 0; });

      spec.arcExist(desc,[=](const auto& p,const auto& c,auto x,int v,bool)->bool{
                          return p.at(ps[v-min]) < values[v];
                       });

      lambdaMap d0 = toDict(minFDom,minLDom,ps,
                            [min,ps] (int i,int pi) -> auto {
                               return tDesc({pi},[=] (auto& out,const auto& p,auto x, const auto& val,bool up) {
                                                    out.set(pi,p.at(pi) + ((val.singleton() - min) == i));
                                                 });
                            });
      spec.transitionDown(d0);
      lambdaMap d1 = toDict(maxFDom,maxLDom,ps,
                            [dz,min,ps] (int i,int pi) -> auto {
                               return tDesc({pi},[=] (auto& out,const auto& p,auto x, const auto& val,bool up) {
                                                    out.set(pi,p.at(pi) + ((val.singleton() - min) == (i - dz)));
                                                 });
                            });
      spec.transitionDown(d1);

      for(ORInt i = minFDom; i <= minLDom; i++){
         int p = ps[i];
         spec.addRelaxation(p,[p](auto& out,auto l,auto r)  { out.set(p,std::min(l.at(p),r.at(p)));});
      }

      for(ORInt i = maxFDom; i <= maxLDom; i++){
         int p = ps[i];
         spec.addRelaxation(p,[p](auto& out,auto l,auto r) { out.set(p,std::max(l.at(p),r.at(p)));});
      }
   }

   void gccMDD2(MDDSpec& spec,const Factory::Veci& vars, const std::map<int,int>& lb, const std::map<int,int>& ub)
   {
      spec.append(vars);
      int sz = (int) vars.size();
      auto udom = domRange(vars);
      int dz = udom.second - udom.first + 1;
      int minFDom = 0,      minLDom = dz-1;
      int maxFDom = dz,     maxLDom = dz*2-1;
      int minFDomUp = dz*2, minLDomUp = dz*3-1;
      int maxFDomUp = dz*3, maxLDomUp = dz*4-1;
      int min = udom.first;
      ValueMap<int> valuesLB(udom.first, udom.second,0,lb);
      ValueMap<int> valuesUB(udom.first, udom.second,0,ub);
      auto desc = spec.makeConstraintDescriptor(vars,"gccMDD");

      std::vector<int> ps = spec.addStates(desc, minFDom, maxLDomUp, sz,[] (int i) -> int { return 0; });

      spec.arcExist(desc,[=](const auto& p,const auto& c,auto x,int v,bool up)->bool{
        bool cond = true;

        int minIdx = v - min;
        int maxIdx = maxFDom + v - min;
        int minIdxUp = minFDomUp + v - min;
        int maxIdxUp = maxFDomUp + v - min;

        if (up) {
          // check LB and UB thresholds when value v is assigned:
          cond = cond && (p.at(ps[minIdx]) + 1 + c.at(ps[minIdxUp]) <= valuesUB[v])
            && (p.at(ps[maxIdx]) + 1 + c.at(ps[maxIdxUp]) >= valuesLB[v]);
          // check LB and UB thresholds for other values, when they are not assigned:
          for (int i=min; i<v; i++) {
            if (!cond) break;
            cond = cond && (p.at(ps[i-min]) + c.at(ps[minFDomUp+i-min]) <= valuesUB[i])
              && (p.at(ps[maxFDom+i-min]) + c.at(ps[maxFDomUp+i-min]) >= valuesLB[i]);
          }
          for (int i=v+1; i<=minLDom+min; i++) {
            if (!cond) break;
            cond = cond && (p.at(ps[i-min]) + c.at(ps[minFDomUp+i-min]) <= valuesUB[i])
              && (p.at(ps[maxFDom+i-min]) + c.at(ps[maxFDomUp+i-min]) >= valuesLB[i]);
          }
        }
        else {
          cond = (p.at(ps[minIdx]) + 1 <= valuesUB[v]);
        }
        return cond;
      });

      spec.nodeExist(desc,[=](const auto& p) {
      	  // check global validity: can we still satisfy all lower bounds?
      	  int remainingLB=0;
      	  int fixedValues=0;
      	  for (int i=0; i<=minLDom; i++) {
      	    remainingLB += std::max(0, valuesLB[i+min] - (p.at(ps[i]) + p.at(ps[minFDomUp+i])));
	    fixedValues += p.at(ps[i]) + p.at(ps[minFDomUp+i]);
	  }
      	  return (fixedValues+remainingLB<=sz);
      	});
      spec.transitionDown(toDict(minFDom,minLDom,
                                 [min,ps] (int i) {
                                    return tDesc({ps[i]},[=](auto& out,const auto& p,auto x,const auto& val,bool up) {
                                                            int tmp = p.at(ps[i]);
                                                            if (val.isSingleton() && (val.singleton() - min) == i) tmp++;
                                                            out.set(ps[i], tmp);
                                                         });
                                 }));
      spec.transitionDown(toDict(maxFDom,maxLDom,
                                 [min,ps,maxFDom](int i) {
                                    return tDesc({ps[i]},[=](auto& out,const auto& p,auto x,const auto& val,bool up) {
                                                            out.set(ps[i], p.at(ps[i])+val.contains(i-maxFDom+min));
                                                         });
                                 }));

      spec.transitionUp(toDict(minFDomUp,minLDomUp,
                               [min,ps,minFDomUp] (int i) {
                                  return tDesc({ps[i]},[=](auto& out,const auto& c,auto x,const auto& val,bool up) {
                                    out.set(ps[i], c.at(ps[i]) + (val.isSingleton() && (val.singleton() - min + minFDomUp == i)));
                                  });
                               }));
      spec.transitionUp(toDict(maxFDomUp,maxLDomUp,
                               [min,ps,maxFDomUp](int i) {
                                 return tDesc({ps[i]},[=](auto& out,const auto& c,auto x,const auto& val,bool up) {
                                   out.set(ps[i], c.at(ps[i])+val.contains(i-maxFDomUp+min));
                                 });
                               }));

      for(ORInt i = minFDom; i <= minLDom; i++){
         int p = ps[i];
         spec.addRelaxation(p,[p](auto& out,auto l,auto r)  { out.set(p,std::min(l.at(p),r.at(p)));});
      }

      for(ORInt i = maxFDom; i <= maxLDom; i++){
         int p = ps[i];
         spec.addRelaxation(p,[p](auto& out,auto l,auto r) { out.set(p,std::max(l.at(p),r.at(p)));});
      }

      for(ORInt i = minFDomUp; i <= minLDomUp; i++){
         int p = ps[i];
         spec.addRelaxation(p,[p](auto& out,auto l,auto r)  { out.set(p,std::min(l.at(p),r.at(p)));});
      }

      for(ORInt i = maxFDomUp; i <= maxLDomUp; i++){
         int p = ps[i];
         spec.addRelaxation(p,[p](auto& out,auto l,auto r) { out.set(p,std::max(l.at(p),r.at(p)));});
      }
   }
}
