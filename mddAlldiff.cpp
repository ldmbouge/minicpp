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

   void allDiffMDD(MDDSpec& mdd, const Factory::Veci& vars)
   {
      mdd.append(vars);
      auto d = mdd.makeConstraintDescriptor(vars,"allDiffMdd");
      auto udom = domRange(vars);
      int minDom = udom.first;
      const int n    = (int)vars.size();
      const int all  = mdd.addDownBSState(d,udom.second - udom.first + 1,0);
      const int some = mdd.addDownBSState(d,udom.second - udom.first + 1,0);
      const int len  = mdd.addDownState(d,0,vars.size());
      const int allu = mdd.addUpBSState(d,udom.second - udom.first + 1,0);
      const int someu = mdd.addUpBSState(d,udom.second - udom.first + 1,0);

      mdd.transitionDown(all,{all},{},[minDom,all](auto& out,const auto& inDown,const auto& inCombined,const auto& var,const auto& val,bool up) noexcept {
                               out.setProp(all,inDown);
                               if (val.size()==1)
                                  out.getBS(all).set(val.singleton() - minDom);
                            });
      mdd.transitionDown(some,{some},{},[minDom,some](auto& out,const auto& inDown,const auto& inCombined,const auto& var,const auto& val,bool up) noexcept {
                                out.setProp(some,inDown);
                                MDDBSValue sv(out.getBS(some));
                                for(auto v : val)
                                   sv.set(v - minDom);
                            });
      mdd.transitionDown(len,{len},{},[len](auto& out,const auto& inDown,const auto& inCombined,const auto& var,const auto& val,bool up) noexcept {
                                      out.set(len,inDown[len] + 1);
                                   });
      mdd.transitionUp(allu,{allu},{},[minDom,allu](auto& out,const auto& inUp,const auto& inCombined,const auto& var,const auto& val,bool up) noexcept {
                               out.setProp(allu,inUp);
                               if (val.size()==1)
                                  out.getBS(allu).set(val.singleton() - minDom);
                            });
      mdd.transitionUp(someu,{someu},{},[minDom,someu](auto& out,const auto& inUp,const auto& inCombined,const auto& var,const auto& val,bool up) noexcept {
                                out.setProp(someu,inUp);
                                MDDBSValue sv(out.getBS(someu));
                                for(auto v : val)
                                   sv.set(v - minDom);
                             });

      mdd.addRelaxationDown(all,[all](auto& out,const auto& l,const auto& r) noexcept    {
                               out.getBS(all).setBinAND(l.getBS(all),r.getBS(all));
                            });
      mdd.addRelaxationDown(some,[some](auto& out,const auto& l,const auto& r) noexcept    {
                                out.getBS(some).setBinOR(l.getBS(some),r.getBS(some));
                            });
      mdd.addRelaxationDown(len,[len](auto& out,const auto& l,const auto& r)   noexcept  { out.set(len,l[len]);});
      mdd.addRelaxationUp(allu,[allu](auto& out,const auto& l,const auto& r)  noexcept   {
                               out.getBS(allu).setBinAND(l.getBS(allu),r.getBS(allu));
                            });
      mdd.addRelaxationUp(someu,[someu](auto& out,const auto& l,const auto& r)  noexcept   {
                                out.getBS(someu).setBinOR(l.getBS(someu),r.getBS(someu));
                            });

      mdd.arcExist(d,[minDom,some,all,len,someu,allu,n](const auto& pDown,const auto& pCombined,const auto& cUp,const auto& cCombined,const auto& var,const auto& val,bool up) noexcept -> bool  {
                      MDDBSValue sbs = pDown.getBS(some);
                      const int ofs = val - minDom;
                      const bool notOk = pDown.getBS(all).getBit(ofs) || (sbs.getBit(ofs) && sbs.cardinality() == pDown[len]);
                      if (notOk) return false;
                      bool upNotOk = false,mixNotOk = false;
                      if (up) {
                         MDDBSValue subs = cUp.getBS(someu);
                         upNotOk = cUp.getBS(allu).getBit(ofs) || (subs.getBit(ofs) && subs.cardinality() == n - cUp[len]);
                         if (upNotOk) return false;
                         MDDBSValue both((char*)alloca(sizeof(unsigned long long)*subs.nbWords()),subs.nbWords());
                         both.setBinOR(subs,sbs).set(ofs);
                         mixNotOk = both.cardinality() < n;
                      }
                      return !mixNotOk;
                   });
      mdd.equivalenceClassValue([some,all,len](const auto& down, const auto& up) -> int {
          return (down.getBS(some).cardinality() - down.getBS(all).cardinality() < down[len]/2);
      });
   }
}
