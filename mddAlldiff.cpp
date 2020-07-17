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
      const int all  = mdd.addBSState(d,udom.second - udom.first + 1,0);
      const int some = mdd.addBSState(d,udom.second - udom.first + 1,0);
      const int len  = mdd.addState(d,0,vars.size());
      const int allu = mdd.addBSState(d,udom.second - udom.first + 1,0);
      const int someu = mdd.addBSState(d,udom.second - udom.first + 1,0);
      
      mdd.transitionDown(all,{all},[minDom,all](auto& out,const auto& in,const auto& var,const auto& val,bool up) noexcept {
                               out.setProp(all,in);
                               if (val.size()==1)
                                  out.getBS(all).set(val.singleton() - minDom);
                            });
      mdd.transitionDown(some,{some},[minDom,some](auto& out,const auto& in,const auto& var,const auto& val,bool up) noexcept {
                                out.setProp(some,in);
                                MDDBSValue sv(out.getBS(some));
                                for(auto v : val)
                                   sv.set(v - minDom);
                            });
      mdd.transitionDown(len,{len},[len](auto& out,const auto& in,const auto& var,const auto& val,bool up) noexcept {
                                      out.set(len,in[len] + 1);
                                   });
      mdd.transitionUp(allu,{allu},[minDom,allu](auto& out,const auto& in,const auto& var,const auto& val,bool up) noexcept {
                               out.setProp(allu,in);
                               if (val.size()==1)
                                  out.getBS(allu).set(val.singleton() - minDom);
                            });
      mdd.transitionUp(someu,{someu},[minDom,someu](auto& out,const auto& in,const auto& var,const auto& val,bool up) noexcept {
                                out.setProp(someu,in);
                                MDDBSValue sv(out.getBS(someu));
                                for(auto v : val)
                                   sv.set(v - minDom);                                 
                             });
      
      mdd.addRelaxation(all,[all](auto& out,const auto& l,const auto& r) noexcept    {
                               out.getBS(all).setBinAND(l.getBS(all),r.getBS(all));
                            });
      mdd.addRelaxation(some,[some](auto& out,const auto& l,const auto& r) noexcept    {
                                out.getBS(some).setBinOR(l.getBS(some),r.getBS(some));
                            });
      mdd.addRelaxation(len,[len](auto& out,const auto& l,const auto& r)   noexcept  { out.set(len,l[len]);});
      mdd.addRelaxation(allu,[allu](auto& out,const auto& l,const auto& r)  noexcept   {
                               out.getBS(allu).setBinAND(l.getBS(allu),r.getBS(allu));
                            });
      mdd.addRelaxation(someu,[someu](auto& out,const auto& l,const auto& r)  noexcept   {
                                out.getBS(someu).setBinOR(l.getBS(someu),r.getBS(someu));
                            });

      mdd.arcExist(d,[minDom,some,all,len,someu,allu,n](const auto& p,const auto& c,const auto& var,const auto& val,bool up) noexcept -> bool  {
                      MDDBSValue sbs = p.getBS(some);
                      const int ofs = val - minDom;
                      const bool notOk = p.getBS(all).getBit(ofs) || (sbs.getBit(ofs) && sbs.cardinality() == p[len]);
                      if (notOk) return false;
                      bool upNotOk = false,mixNotOk = false;
                      if (up) {
                         MDDBSValue subs = c.getBS(someu);
                         upNotOk = c.getBS(allu).getBit(ofs) || (subs.getBit(ofs) && subs.cardinality() == n - c[len]);
                         if (upNotOk) return false;
                         MDDBSValue both((char*)alloca(sizeof(unsigned long long)*subs.nbWords()),subs.nbWords());
                         both.setBinOR(subs,sbs).set(ofs);
                         mixNotOk = both.cardinality() < n;
                      }
                      return !mixNotOk;
                   });
      mdd.equivalenceClassValue([some,all,len](const auto& p, const auto& c, var<int>::Ptr var, int val) -> int {
          return (c.getBS(some).cardinality() - c.getBS(all).cardinality() < p[len]/2);
      });
   }
}
