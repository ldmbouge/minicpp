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
   MDDCstrDesc::Ptr seqMDD(MDD::Ptr m,const Factory::Veci& vars, int len, int lb, int ub, std::set<int> rawValues)
   {
      MDDSpec& spec = m->getSpec();
      ValueSet values(rawValues);
      auto desc = spec.makeConstraintDescriptor(vars,"seqMDD");

      const auto minWin = spec.downSWState(desc,len,-1,0,MinFun);
      const auto maxWin = spec.downSWState(desc,len,-1,0,MaxFun);

      spec.arcExist(desc,[=](const auto& parent,const auto& child,const auto& x,int v) {
         bool inS = values.member(v);
         auto min = parent.down[minWin];
         auto max = parent.down[maxWin];
         int minv = max.first() - min.last() + inS;
         return (min.last() < 0 &&  minv >= lb && min.first() + inS              <= ub)
            ||  (min.last() >= 0 && minv >= lb && min.first() - max.last() + inS <= ub);
      });
      spec.transitionDown(desc,minWin,{minWin},{},[=](auto& out,const auto& parent,const auto&,const auto& val) {
         bool allMembers = val.allInside(values);
         MDDSWin<short> outWin = out[minWin];
         outWin.assignSlideBy(parent.down[minWin],1);
         outWin.setFirst(parent.down[minWin].first() + allMembers);
      });
      spec.transitionDown(desc,maxWin,{maxWin},{},[=](auto& out,const auto& parent,const auto&,const auto& val) {
         bool oneMember = val.memberInside(values);
         MDDSWin<short> outWin = out[maxWin];
         outWin.assignSlideBy(parent.down[maxWin],1);
         outWin.setFirst(parent.down[maxWin].first() + oneMember);
      });
      return desc;
   }

   MDDCstrDesc::Ptr seqMDD2(MDD::Ptr m,const Factory::Veci& vars, int len, int lb, int ub, std::set<int> rawValues)
   {
      MDDSpec& spec = m->getSpec();
      ValueSet values(rawValues);
      auto desc = spec.makeConstraintDescriptor(vars,"seqMDD");

      const auto minWin = spec.downSWState(desc,len,-1,0,MinFun);
      const auto maxWin = spec.downSWState(desc,len,-1,0,MaxFun);
      const auto pnb    = spec.downIntState(desc,0,INT_MAX,MinFun); // init @ 0, largest value is number of variables. 

      spec.transitionDown(desc,minWin,{minWin},{},[values,minWin](auto& out,const auto& parent,const auto&,const auto& val) {
         bool allMembers = val.allInside(values);
         MDDSWin<short> outWin = out[minWin];
         outWin.assignSlideBy(parent.down[minWin],1);
         outWin.setFirst(parent.down[minWin].first() + allMembers);
      });
      spec.transitionDown(desc,maxWin,{maxWin},{},[values,maxWin](auto& out,const auto& parent,const auto&,const auto& val) {
         bool oneMember = val.memberInside(values);
         MDDSWin<short> outWin = out[maxWin];
         outWin.assignSlideBy(parent.down[maxWin],1);
         outWin.setFirst(parent.down[maxWin].first() + oneMember);
      });
      spec.transitionDown(desc,pnb,{pnb},{},[pnb](auto& out,const auto& parent,const auto& x,const auto& val) {
         out[pnb] = parent.down[pnb]+1;
      });

      spec.arcExist(desc,
                    [=](const auto& parent,const auto& child,const auto& x,int v) {
                       bool inS = values.member(v);
                       MDDSWin<short> min = parent.down[minWin];
                       MDDSWin<short> max = parent.down[maxWin];
                       if (parent.down[pnb] >= len - 1) {
                          bool c0 = max.first() + inS - min.last() >= lb;
                          bool c1 = min.first() + inS - max.last() <= ub;
                          return c0 && c1;
                       } else {
                          bool c0 = len - (parent.down[pnb]+1) + max.first() + inS >= lb;
                          bool c1 =                              min.first() + inS <= ub;
                          return c0 && c1;
                       }
                    });
      return desc;      
   }

   MDDCstrDesc::Ptr seqMDD3(MDD::Ptr m,const Factory::Veci& vars, int len, int lb, int ub, std::set<int> rawValues)
   {
      MDDSpec& spec = m->getSpec();
      const int nbVars = (int)vars.size();
      ValueSet values(rawValues);
      auto desc = spec.makeConstraintDescriptor(vars,"seqMDD");

      const auto YminDown = spec.downIntState(desc, 0, INT_MAX,MinFun);
      const auto YmaxDown = spec.downIntState(desc, 0, INT_MAX,MaxFun);
      const auto YminUp = spec.upIntState(desc, 0, INT_MAX,MinFun);
      const auto YmaxUp = spec.upIntState(desc, nbVars, INT_MAX,MaxFun);
      const auto YminCombined = spec.combinedIntState(desc, 0, INT_MAX,MinFun);
      const auto YmaxCombined = spec.combinedIntState(desc, 0, INT_MAX,MaxFun);
      const auto AminWin = spec.downSWState(desc,len,-1,0,MinFun);
      const auto AmaxWin = spec.downSWState(desc,len,-1,0,MaxFun);
      const auto DminWin = spec.upSWState(desc,len,-1,0,MinFun);
      const auto DmaxWin = spec.upSWState(desc,len,-1,0,MaxFun);
      const auto N       = spec.downIntState(desc, 0, INT_MAX,MinFun);
      const auto Nup     = spec.upIntState(desc, 0, INT_MAX,MinFun);
      const auto Exact   = spec.downIntState(desc, 1, INT_MAX,MinFun);

      // down transitions
      spec.transitionDown(desc,AminWin,{AminWin},{YminCombined},[=](auto& out,const auto& parent,const auto& x,const auto& val) {
         MDDSWin<short> outWin = out[AminWin];
         outWin.assignSlideBy(parent.down[AminWin],1);        
         auto pYmin = parent.comb[YminCombined];
         outWin.setFirst(pYmin);
      });
      spec.transitionDown(desc,AmaxWin,{AmaxWin},{YmaxCombined},[=](auto& out,const auto& parent,const auto& x,const auto& val) {
         MDDSWin<short> outWin = out[AmaxWin];
         outWin.assignSlideBy(parent.down[AmaxWin],1);
         auto pYmax = parent.comb[YmaxCombined];
         outWin.setFirst(pYmax);
      });
      spec.transitionDown(desc,YminDown,{},{YminCombined},[=](auto& out,const auto& parent,const auto& x,const auto& val) {
         assert(parent.down[YminDown] <= parent.down[YmaxDown]);
         bool hasMemberOutS = val.memberOutside(values);
         auto pYmin = parent.comb[YminCombined];
         int minVal = pYmin + !hasMemberOutS;
         out[YminDown] = minVal;
      });

      spec.transitionDown(desc,YmaxDown,{},{YmaxCombined},[=](auto& out,const auto& parent,const auto& x,const auto& val) {
         assert(parent.down[YminDown] <= parent.down[YmaxDown]);
         bool hasMemberInS = val.memberInside(values);
         auto pYmax = parent.comb[YmaxCombined];
         int maxVal = pYmax + hasMemberInS;
         out[YmaxDown] = maxVal;
      });

      spec.transitionDown(desc,N,{N},{},[N](auto& out,const auto& parent,const auto& x,const auto& valo) {
         out[N] = parent.down[N]+1;
      });
      spec.transitionDown(desc,Exact,{Exact},{},[Exact,values](auto& out,const auto& parent,const auto& x,const auto& val) {
         out[Exact] = (parent.down[Exact]==1) && (val.memberOutside(values) != val.memberInside(values));
      });

      // up transitions
      spec.transitionUp(desc,DminWin,{DminWin},{YminCombined},[DminWin,YminCombined](auto& out,const auto& child,const auto&,const auto& val) {
         MDDSWin<short> outWin = out[DminWin];
         outWin.assignSlideBy(child.up[DminWin],1);
         auto cYmin = child.comb[YminCombined];
         outWin.setFirst(cYmin);
      });
      spec.transitionUp(desc,DmaxWin,{DmaxWin},{YmaxCombined},[DmaxWin,YmaxCombined](auto& out,const auto& child,const auto&,const auto& val) {
         MDDSWin<short> outWin = out[DmaxWin];
         outWin.assignSlideBy(child.up[DmaxWin],1);
         auto cYmax = child.comb[YmaxCombined];
         outWin.setFirst(cYmax);
      });
      
      spec.transitionUp(desc,YminUp,{},{YminCombined},[YminUp,values,YminCombined](auto& out,const auto& child,const auto&,const auto& val) {
         bool hasMemberInS = val.memberInside(values);
         const int cYmin = child.comb[YminCombined];
         const int minVal = std::max(cYmin - hasMemberInS,0);
         out[YminUp] = minVal;
      });

      spec.transitionUp(desc,YmaxUp,{},{YmaxCombined},[YmaxUp,values,YmaxCombined,Nup,nbVars](auto& out,const auto& child,const auto&,const auto& val) {
         bool hasMemberOutS = val.memberOutside(values);
         const int cYmax = child.comb[YmaxCombined];
         const int maxVal = std::min(cYmax - !hasMemberOutS, nbVars - child.up[Nup] - 1);
         out[YmaxUp] = maxVal;
      });

      spec.transitionUp(desc,Nup,{Nup},{},[Nup](auto& out,const auto& child,const auto& x,const auto& val) {
         out[Nup] = child.up[Nup]+1;
      });

      spec.updateNode(desc,YminCombined,{AminWin,YminDown,N},{DminWin,YminUp,Nup},[=](auto& combined,const auto& n) {
         int minVal = n.down[YminDown]; 
         if (n.down[N] >= len) {
            auto Amin = n.down[AminWin];
            minVal = std::max(lb + Amin.last(),minVal);
         }
         if (n.up[Nup]) {
            minVal = std::max(minVal, (int) n.up[YminUp]);
           if (n.up[Nup] >= len) {
             auto Dmin = n.up[DminWin];
             minVal = std::max(Dmin.last() - ub,minVal);
           }
         }
         combined[YminCombined] = minVal;
      });
      spec.updateNode(desc,YmaxCombined,{AmaxWin,YmaxDown,N},{DmaxWin,YmaxUp,Nup},[=](auto& combined,const auto& n) {
         int maxVal = n.down[YmaxDown]; 
         if (n.down[N] >= len) {
            auto Amax = n.down[AmaxWin];
            maxVal = std::min(ub + Amax.last(),maxVal);
         }
         if (n.up[Nup]) {
            maxVal = std::min(maxVal, (int)n.up[YmaxUp]);
            if (n.up[Nup] >= len) {
               auto Dmax = n.up[DmaxWin];
               maxVal = std::min(Dmax.last() - lb,maxVal);
            }
         }
         combined[YmaxCombined] = maxVal;
      });
      
      spec.nodeExist([=](const auto& n) {
         bool rv = ( (n.comb[YminCombined] <= n.comb[YmaxCombined]) &&
                     (n.comb[YmaxCombined] >= 0) &&
                     (n.comb[YmaxCombined] <= n.down[N]) &&
                     (n.comb[YminCombined] >= 0) &&
                     (n.comb[YminCombined] <= n.down[N]) );
         return rv;
      });

      // arc definitions
      spec.arcExist(desc,[values,YminCombined,YmaxCombined](const auto& parent,const auto& child,const auto& x,int v) {
         bool inS = values.member(v);
         bool c0 = (parent.comb[YminCombined] + inS <= child.comb[YmaxCombined]);
         bool c1 = (parent.comb[YmaxCombined] + inS >= child.comb[YminCombined]);
         return c0 && c1;
      });
      
      spec.splitOnLargest([Exact](const auto& n) {
                             return (double)(n.getDownState()[Exact]);
                          });

      spec.equivalenceClassValue([YminDown,YmaxDown](const auto& down, const auto& up) -> int {
         return down[YmaxDown] - down[YminDown] < 4;
      });
      return desc;
   }
}
