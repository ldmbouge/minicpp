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
      spec.append(vars);
      int sz = (int) vars.size();
      auto udom = domRange(vars);
      int dz = udom.second - udom.first + 1;
      int minFDom = 0, minLDom = dz-1;
      int maxFDom = dz,maxLDom = dz*2-1;
      int min = udom.first;
      ValueMap<int> values(udom.first, udom.second,0,ub);
      auto desc = spec.makeConstraintDescriptor(vars,"gccMDD");

      std::vector<int> ps = spec.addDownStates(desc,minFDom, maxLDom,sz,[] (int i) -> int { return 0; });

      spec.arcExist(desc,[=](const auto& parent,const auto& child,auto x,int v)->bool{
         return parent.down.at(ps[v-min]) < values[v];
      });

      lambdaMap d0 = toDict(minFDom,minLDom,ps,
                            [min,ps] (int i,int pi) -> auto {
                               return tDesc({pi},{},[=] (auto& out,const auto& parent,auto x, const auto& val,bool up) {
                                  out.set(pi,parent.down.at(pi) + ((val.singleton() - min) == i));
                               });
                            });
      spec.transitionDown(desc,d0);
      lambdaMap d1 = toDict(maxFDom,maxLDom,ps,
                            [dz,min,ps] (int i,int pi) -> auto {
                               return tDesc({pi},{},[=] (auto& out,const auto& parent,auto x, const auto& val,bool up) {
                                  out.set(pi,parent.down.at(pi) + ((val.singleton() - min) == (i - dz)));
                               });
                            });
      spec.transitionDown(desc,d1);

      for(int i = minFDom; i <= minLDom; i++){
         spec.addRelaxationDown(desc,ps[i],[p = ps[i]](auto& out,auto l,auto r)  { out.set(p,std::min(l.at(p),r.at(p)));});
      }

      for(int i = maxFDom; i <= maxLDom; i++){
         spec.addRelaxationDown(desc,ps[i],[p = ps[i]](auto& out,auto l,auto r) { out.set(p,std::max(l.at(p),r.at(p)));});
      }
      return desc;
   }

   MDDCstrDesc::Ptr gccMDD2(MDD::Ptr m,const Factory::Veci& vars, const std::map<int,int>& lb, const std::map<int,int>& ub)
   {
      MDDSpec& spec = m->getSpec();
      spec.append(vars);
      int sz = (int) vars.size();
      auto udom = domRange(vars);
      int dz = udom.second - udom.first + 1;
      int minFDom = 0,      minLDom = dz-1;
      int maxFDom = dz,     maxLDom = dz*2-1;
      int minFDomUp = 0, minLDomUp = dz-1;
      int maxFDomUp = dz, maxLDomUp = dz*2-1;
      int min = udom.first;
      ValueMap<int> valuesLB(udom.first, udom.second,0,lb);
      ValueMap<int> valuesUB(udom.first, udom.second,0,ub);
      auto desc = spec.makeConstraintDescriptor(vars,"gccMDD");

      std::vector<int> downPs = spec.addDownStates(desc, minFDom, maxLDom, sz,[] (int i) -> int { return 0; });
      std::vector<int> upPs = spec.addUpStates(desc, minFDomUp, maxLDomUp, sz,[] (int i) -> int { return 0; });

      spec.arcExist(desc,[=](const auto& parent,const auto& child,auto x,int v)->bool{
        bool cond = true;

        int minIdx = v - min;
        int maxIdx = maxFDom + v - min;
        int minIdxUp = minFDomUp + v - min;
        int maxIdxUp = maxFDomUp + v - min;

        // check LB and UB thresholds when value v is assigned:
        cond = cond && (parent.down.at(downPs[minIdx]) + 1 + child.up.at(upPs[minIdxUp]) <= valuesUB[v])
           && (parent.down.at(downPs[maxIdx]) + 1 + child.up.at(upPs[maxIdxUp]) >= valuesLB[v]);
        // check LB and UB thresholds for other values, when they are not assigned:
        for (int i=min; i<v; i++) {
           if (!cond) break;
           cond = cond && (parent.down.at(downPs[i-min]) + child.up.at(upPs[i-min]) <= valuesUB[i])
              && (parent.down.at(downPs[maxFDom+i-min]) + child.up.at(upPs[i-min]) >= valuesLB[i]);
        }
        for (int i=v+1; i<=minLDom+min; i++) {
           if (!cond) break;
           cond = cond && (parent.down.at(downPs[i-min]) + child.up.at(upPs[i-min]) <= valuesUB[i])
              && (parent.down.at(downPs[maxFDom+i-min]) + child.up.at(upPs[i-min]) >= valuesLB[i]);
        }
        return cond;
      });

      spec.nodeExist([=](const auto& n) {
      	  // check global validity: can we still satisfy all lower bounds?
      	  int remainingLB=0;
      	  int fixedValues=0;
      	  for (int i=0; i<=minLDom; i++) {
      	    remainingLB += std::max(0, valuesLB[i+min] - (n.down.at(downPs[i]) + n.up.at(upPs[i])));
            fixedValues += n.down.at(downPs[i]) + n.up.at(upPs[i]);
          }
      	  return (fixedValues+remainingLB<=sz);
      });
      spec.transitionDown(desc,toDict(minFDom,minLDom,
                                 [min,downPs] (int i) {
                                    return tDesc({downPs[i]},{},[=](auto& out,const auto& parent,auto x,const auto& val,bool up) {
                                       int tmp = parent.down.at(downPs[i]);
                                       if (val.isSingleton() && (val.singleton() - min) == i) tmp++;
                                       out.set(downPs[i], tmp);
                                    });
                                 }));
      spec.transitionDown(desc,toDict(maxFDom,maxLDom,
                                 [min,downPs,maxFDom](int i) {
                                    return tDesc({downPs[i]},{},[=](auto& out,const auto& parent,auto x,const auto& val,bool up) {
                                       out.set(downPs[i], parent.down.at(downPs[i])+val.contains(i-maxFDom+min));
                                    });
                                 }));
      
      spec.transitionUp(desc,toDict(minFDomUp,minLDomUp,
                               [min,upPs,minFDomUp] (int i) {
                                  return tDesc({upPs[i]},{},[=](auto& out,const auto& child,auto x,const auto& val,bool up) {
                                    out.set(upPs[i], child.up.at(upPs[i]) + (val.isSingleton() && (val.singleton() - min + minFDomUp == i)));
                                  });
                               }));
      spec.transitionUp(desc,toDict(maxFDomUp,maxLDomUp,
                               [min,upPs,maxFDomUp](int i) {
                                 return tDesc({upPs[i]},{},[=](auto& out,const auto& child,auto x,const auto& val,bool up) {
                                    out.set(upPs[i], child.up.at(upPs[i])+val.contains(i-maxFDomUp+min));
                                 });
                               }));

      for(int i = minFDom; i <= minLDom; i++){
         int p = downPs[i];
         spec.addRelaxationDown(desc,p,[p](auto& out,auto l,auto r)  { out.set(p,std::min(l.at(p),r.at(p)));});
      }

      for(int i = maxFDom; i <= maxLDom; i++){
         int p = downPs[i];
         spec.addRelaxationDown(desc,p,[p](auto& out,auto l,auto r) { out.set(p,std::max(l.at(p),r.at(p)));});
      }

      for(int i = minFDomUp; i <= minLDomUp; i++){
         int p = upPs[i];
         spec.addRelaxationUp(desc,p,[p](auto& out,auto l,auto r)  { out.set(p,std::min(l.at(p),r.at(p)));});
      }

      for(int i = maxFDomUp; i <= maxLDomUp; i++){
         int p = upPs[i];
         spec.addRelaxationUp(desc,p,[p](auto& out,auto l,auto r) { out.set(p,std::max(l.at(p),r.at(p)));});
      }
      return desc;
   }
}
