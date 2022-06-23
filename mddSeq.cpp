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
   void seqMDD(MDDSpec& spec,const Factory::Veci& vars, int len, int lb, int ub, std::set<int> rawValues)
   {
      spec.append(vars);
      ValueSet values(rawValues);
      auto desc = spec.makeConstraintDescriptor(vars,"seqMDD");

      int minWin = spec.addDownSWState(desc,len,-1,0,MinFun);
      int maxWin = spec.addDownSWState(desc,len,-1,0,MaxFun);

      spec.arcExist(desc,
                    [minWin,maxWin,lb,ub,values] (const auto& parent,const auto& child,const auto& x,int v) {
                       bool inS = values.member(v);
                       auto min = parent.down.getSW(minWin);
                       auto max = parent.down.getSW(maxWin);
                       int minv = max.first() - min.last() + inS;
                       return (min.last() < 0 &&  minv >= lb && min.first() + inS              <= ub)
                          ||  (min.last() >= 0 && minv >= lb && min.first() - max.last() + inS <= ub);
                    });
      spec.transitionDown(minWin,{minWin},{},
                          [values,minWin](auto& out,const auto& parent,const auto& x,const auto& val,bool up) {
                             bool allMembers = val.allInside(values);
                             MDDSWin<short> outWin = out.getSW(minWin);
                             outWin.assignSlideBy(parent.down.getSW(minWin),1);
                             outWin.setFirst(parent.down.getSW(minWin).first() + allMembers);
                          });
      spec.transitionDown(maxWin,{maxWin},{},
                          [values,maxWin](auto& out,const auto& parent,const auto& x,const auto& val,bool up) {
                             bool oneMember = val.memberInside(values);
                             MDDSWin<short> outWin = out.getSW(maxWin);
                             outWin.assignSlideBy(parent.down.getSW(maxWin),1);
                             outWin.setFirst(parent.down.getSW(maxWin).first() + oneMember);
                          });
   }

   void seqMDD2(MDDSpec& spec,const Factory::Veci& vars, int len, int lb, int ub, std::set<int> rawValues)
   {
      spec.append(vars);
      ValueSet values(rawValues);
      auto desc = spec.makeConstraintDescriptor(vars,"seqMDD");

      int minWin = spec.addDownSWState(desc,len,-1,0,MinFun);
      int maxWin = spec.addDownSWState(desc,len,-1,0,MaxFun);
      int pnb    = spec.addDownState(desc,0,INT_MAX,MinFun); // init @ 0, largest value is number of variables. 

      spec.transitionDown(minWin,{minWin},{},[values,minWin](auto& out,const auto& parent,const auto& x,const auto& val,bool up) {
                                             bool allMembers = val.allInside(values);
                                             MDDSWin<short> outWin = out.getSW(minWin);
                                             outWin.assignSlideBy(parent.down.getSW(minWin),1);
                                             outWin.setFirst(parent.down.getSW(minWin).first() + allMembers);
                                          });
      spec.transitionDown(maxWin,{maxWin},{},[values,maxWin](auto& out,const auto& parent,const auto& x,const auto& val,bool up) {
                                             bool oneMember = val.memberInside(values);
                                             MDDSWin<short> outWin = out.getSW(maxWin);
                                             outWin.assignSlideBy(parent.down.getSW(maxWin),1);
                                             outWin.setFirst(parent.down.getSW(maxWin).first() + oneMember);
                                          });
      spec.transitionDown(pnb,{pnb},{},[pnb](auto& out,const auto& parent,const auto& x,const auto& val,bool up) {
                                       out.setInt(pnb,parent.down[pnb]+1);
                                    });

      spec.arcExist(desc,
                    [=](const auto& parent,const auto& child,const auto& x,int v) -> bool {
                       bool inS = values.member(v);
                       MDDSWin<short> min = parent.down.getSW(minWin);
                       MDDSWin<short> max = parent.down.getSW(maxWin);
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
      
   }

   void seqMDD3(MDDSpec& spec,const Factory::Veci& vars, int len, int lb, int ub, std::set<int> rawValues)
   {
      const int nbVars = (int)vars.size();
      spec.append(vars);
      ValueSet values(rawValues);
      auto desc = spec.makeConstraintDescriptor(vars,"seqMDD");

      const int YminDown = spec.addDownState(desc, 0, INT_MAX,MinFun);
      const int YmaxDown = spec.addDownState(desc, 0, INT_MAX,MaxFun);
      const int YminUp = spec.addUpState(desc, 0, INT_MAX,MinFun);
      const int YmaxUp = spec.addUpState(desc, vars.size(), INT_MAX,MaxFun);
      const int YminCombined = spec.addCombinedState(desc, 0, INT_MAX,MinFun);
      const int YmaxCombined = spec.addCombinedState(desc, 0, INT_MAX,MaxFun);
      const int AminWin = spec.addDownSWState(desc,len,-1,0,MinFun);
      const int AmaxWin = spec.addDownSWState(desc,len,-1,0,MaxFun);
      const int DminWin = spec.addUpSWState(desc,len,-1,0,MinFun);
      const int DmaxWin = spec.addUpSWState(desc,len,-1,0,MaxFun);
      const int N       = spec.addDownState(desc, 0, INT_MAX,MinFun);
      const int Nup     = spec.addUpState(desc, 0, INT_MAX,MinFun);
      const int Exact   = spec.addDownState(desc, 1, INT_MAX,MinFun);

      // down transitions
      spec.transitionDown(AminWin,{YminDown,AminWin},{YminCombined},[=](auto& out,const auto& parent,const auto& x,const auto& val,bool up) {
         MDDSWin<short> outWin = out.getSW(AminWin);
         outWin.assignSlideBy(parent.down.getSW(AminWin),1);        
         auto pYmin = !up ? parent.down[YminDown] : std::max(parent.down[YminDown],parent.comb[YminCombined]);
         outWin.setFirst(pYmin);
      });
      spec.transitionDown(AmaxWin,{YmaxDown,AmaxWin},{YmaxCombined},[=](auto& out,const auto& parent,const auto& x,const auto& val,bool up) {
         MDDSWin<short> outWin = out.getSW(AmaxWin);
         outWin.assignSlideBy(parent.down.getSW(AmaxWin),1);
         auto pYmax = !up ? parent.down[YmaxDown] : std::min(parent.down[YmaxDown],parent.comb[YmaxCombined]);
         outWin.setFirst(pYmax);
      });      
      spec.transitionDown(YminDown,{YminDown},{YminCombined},[=](auto& out,const auto& parent,const auto& x,const auto& val,bool up) {
         assert(parent.down[YminDown] <= parent.down[YmaxDown]);
         bool hasMemberOutS = val.memberOutside(values);
         auto pYmin = !up ? parent.down[YminDown] : std::max(parent.down[YminDown],parent.comb[YminCombined]);
         int minVal = pYmin + !hasMemberOutS;
         out.setInt(YminDown,minVal);
      });

      spec.transitionDown(YmaxDown,{YmaxDown},{YmaxCombined},[=](auto& out,const auto& parent,const auto& x,const auto& val,bool up) {
         assert(parent.down[YminDown] <= parent.down[YmaxDown]);
         bool hasMemberInS = val.memberInside(values);
         auto pYmax = !up ? parent.down[YmaxDown] : std::min(parent.down[YmaxDown],parent.comb[YmaxCombined]);
         int maxVal = pYmax + hasMemberInS;
         out.setInt(YmaxDown,maxVal);
      });

      spec.transitionDown(N,{N},{},[N](auto& out,const auto& parent,const auto& x,const auto& val,bool) {
         out.setInt(N,parent.down[N]+1);
      });
      spec.transitionDown(Exact,{Exact},{},[Exact,values](auto& out,const auto& parent,const auto& x,const auto& val,bool) {
         out.setInt(Exact, (parent.down[Exact]==1) && (val.memberOutside(values) != val.memberInside(values)));
      });

      // up transitions
      spec.transitionUp(DminWin,{YminUp,DminWin},{YminCombined},[DminWin,YminUp,YminCombined](auto& out,const auto& child,const auto& x,const auto& val,bool up) {
         MDDSWin<short> outWin = out.getSW(DminWin);
         outWin.assignSlideBy(child.up.getSW(DminWin),1);
         auto cYmin = std::max(child.up[YminUp],child.comb[YminCombined]);
         outWin.setFirst(cYmin);
      });
      spec.transitionUp(DmaxWin,{YmaxUp,DmaxWin},{YmaxCombined},[DmaxWin,YmaxUp,YmaxCombined](auto& out,const auto& child,const auto& x,const auto& val,bool up) {
         MDDSWin<short> outWin = out.getSW(DmaxWin);
         outWin.assignSlideBy(child.up.getSW(DmaxWin),1);
         auto cYmax = std::min(child.up[YmaxUp],child.comb[YmaxCombined]);
         outWin.setFirst(cYmax);
      });
      
      spec.transitionUp(YminUp,{YminUp},{YminCombined},[YminUp,values,YminCombined](auto& out,const auto& child,const auto& x,const auto& val,bool up) {
         bool hasMemberInS = val.memberInside(values);
         auto cYmin = std::max(child.up[YminUp],child.comb[YminCombined]);
         int minVal = cYmin - hasMemberInS;
         out.setInt(YminUp,minVal);
      });

      spec.transitionUp(YmaxUp,{YmaxUp},{YmaxCombined},[YmaxUp,values,YmaxCombined](auto& out,const auto& child,const auto& x,const auto& val,bool up) {
         bool hasMemberOutS = val.memberOutside(values);
         auto cYmax = std::min(child.up[YmaxUp],child.comb[YmaxCombined]);
         int maxVal = cYmax - !hasMemberOutS;
         out.setInt(YmaxUp,maxVal);
      });

      spec.transitionUp(Nup,{Nup},{},[Nup](auto& out,const auto& child,const auto& x,const auto& val,bool) {
         out.setInt(Nup,child.up[Nup]+1);
      });

      spec.updateNode(YminCombined,{AminWin,YminDown,N},{DminWin,YminUp,Nup},[=](auto& combined,const auto& n) {
         int minVal = n.down[YminDown]; // [ldm] fix attempt. But not enough.
         if (n.down[N] >= len) {
            auto Amin = n.down.getSW(AminWin);
            minVal = std::max(lb + Amin.last(),minVal);
         }
         if (n.up[Nup]) {
           minVal = std::max(minVal, n.up[YminUp]);
           if (n.down[N] <= nbVars - len) {
             auto Dmin = n.up.getSW(DminWin);
             minVal = std::max(Dmin.last() - ub,minVal);
           }
         }
         combined.setInt(YminCombined,minVal);
      });
      spec.updateNode(YmaxCombined,{AmaxWin,YmaxDown,N},{DmaxWin,YmaxUp,Nup},[=](auto& combined,const auto& n) {
         int maxVal = n.down[YmaxDown]; // fix attempt. But not enough. 
         if (n.down[N] >= len) {
            auto Amax = n.down.getSW(AmaxWin);
            maxVal = std::min(ub + Amax.last(),maxVal);
         }
         if (n.up[Nup]) {
           maxVal = std::min(maxVal, n.up[YmaxUp]);
           if (n.down[N] <= nbVars - len) {
             auto Dmax = n.up.getSW(DmaxWin);
             maxVal = std::min(Dmax.last() - lb,maxVal);
           }
         }
         combined.setInt(YmaxCombined,maxVal);
      });
      
      spec.nodeExist([=](const auto& n) {
         bool rv = ( (n.comb[YminCombined] <= n.comb[YmaxCombined]) &&
                     (n.comb[YmaxCombined] >= 0) &&
                     (n.comb[YmaxCombined] <= n.down[N]) &&
                     (n.comb[YminCombined] >= 0) &&
                     (n.comb[YminCombined] <= n.down[N]) );
         if (rv==false) {
            std::cout << "About to kill node\n";
         }
         return rv;
      });

      // arc definitions
      spec.arcExist(desc,[values,YminCombined,YmaxCombined](const auto& parent,const auto& child,const auto& x,int v) -> bool {
         bool c0 = true,c1 = true,inS = values.member(v);
         c0 = (parent.comb[YminCombined] + inS <= child.comb[YmaxCombined]);
         c1 = (parent.comb[YmaxCombined] + inS >= child.comb[YminCombined]);
         return c0 && c1;
      });
      
      spec.splitOnLargest([Exact](const auto& n) {
                             return (double)(n.getDownState()[Exact]);
                          });

      spec.equivalenceClassValue([YminDown,YmaxDown](const auto& down, const auto& up) -> int {
         return down[YmaxDown] - down[YminDown] < 4;
      });
   }

}
