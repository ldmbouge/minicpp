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
   void amongMDD(MDDSpec& mdd, const Factory::Vecb& x, int lb, int ub,std::set<int> rawValues) {
      mdd.append(x);
      assert(rawValues.size()==1);
      int tv = *rawValues.cbegin();
      auto d = mdd.makeConstraintDescriptor(x,"amongMDD");
      const int minC = mdd.addState(d,0,INT_MAX,MinFun);
      const int maxC = mdd.addState(d,0,INT_MAX,MaxFun);
      const int rem  = mdd.addState(d,(int)x.size(),INT_MAX,MaxFun);
      mdd.arcExist(d,[minC,maxC,rem,tv,ub,lb] (const auto& p,const auto& c,auto var,const auto& val,bool) -> bool {
                        bool vinS = tv == val;// values.member(val);
                        return (p[minC] + vinS <= ub) &&
                           ((p[maxC] + vinS +  p[rem] - 1) >= lb);
                     });         

      mdd.transitionDown(minC,{minC}, [minC,tv] (auto& out,const auto& p,const auto& x, const auto& val,bool up) {
                                         bool allMembers = val.size()==1 && val.singleton() == tv;
                                         out.setInt(minC,p[minC] + allMembers);
                                      });
      mdd.transitionDown(maxC,{maxC},[maxC,tv] (auto& out,const auto& p,const auto& x, const auto& val,bool up) {
                                        bool oneMember = val.contains(tv);
                                        out.setInt(maxC,p[maxC] + oneMember);
                                     });
       mdd.transitionDown(rem,{rem},[rem] (auto& out,const auto& p,const auto& x,const auto& val,bool up) {
                                       out.setInt(rem,p[rem] - 1);
                                    });
       
      mdd.addSimilarity(minC,[minC](auto l,auto r) -> double { return abs(l[minC] - r[minC]); });
      mdd.addSimilarity(maxC,[maxC](auto l,auto r) -> double { return abs(l[maxC] - r[maxC]); });
      mdd.addSimilarity(rem ,[] (auto l,auto r) -> double { return 0; });
      mdd.splitOnLargest([](const auto& in) { return -(double)in.getNumParents();});
   }

   void amongMDD(MDDSpec& mdd, const Factory::Veci& x, int lb, int ub, std::set<int> rawValues) {
      mdd.append(x);
      ValueSet values(rawValues);
      auto d = mdd.makeConstraintDescriptor(x,"amongMDD");
      const int minC = mdd.addState(d,0,INT_MAX,MinFun);
      const int maxC = mdd.addState(d,0,INT_MAX,MaxFun);
      const int rem  = mdd.addState(d,(int)x.size(),INT_MAX,MaxFun);
      if (rawValues.size() == 1) {
         int tv = *rawValues.cbegin();
         mdd.arcExist(d,[minC,maxC,rem,tv,ub,lb] (const auto& p,const auto& c,auto var, const auto& val,bool) -> bool {
                           bool vinS = tv == val;// values.member(val);
                           return (p[minC] + vinS <= ub) &&
                              ((p[maxC] + vinS +  p[rem] - 1) >= lb);
                        });         
      } else {
         mdd.arcExist(d,[=] (const auto& p,const auto& c,var<int>::Ptr var, const auto& val,bool) -> bool {
                           bool vinS = values.member(val);
                           return (p[minC] + vinS <= ub) &&
                              ((p[maxC] + vinS +  p[rem] - 1) >= lb);
                        });
      }

      mdd.transitionDown(minC,{minC},[minC,values] (auto& out,const auto& p,const auto& x, const auto& val,bool up) {
                                bool allMembers = true;
                                for(int v : val) {
                                   allMembers &= values.member(v);
                                   if (!allMembers) break;
                                }
                                out.setInt(minC,p[minC] + allMembers);
                             });
      mdd.transitionDown(maxC,{maxC},[maxC,values] (auto& out,const auto& p,const auto& x, const auto& val,bool up) {
                                bool oneMember = false;
                                for(int v : val) {
                                   oneMember = values.member(v);
                                   if (oneMember) break;
                                }
                                out.setInt(maxC,p[maxC] + oneMember);
                             });
      mdd.transitionDown(rem,{rem},[rem] (auto& out,const auto& p,const auto& x,const auto& val,bool up) { out.setInt(rem,p[rem] - 1);});

      mdd.addSimilarity(minC,[minC](auto l,auto r) -> double { return abs(l[minC] - r[minC]); });
      mdd.addSimilarity(maxC,[maxC](auto l,auto r) -> double { return abs(l[maxC] - r[maxC]); });
      mdd.addSimilarity(rem ,[] (auto l,auto r) -> double { return 0; });
      mdd.splitOnLargest([](const auto& in) { return -(double)in.getNumParents();});
   }

   void amongMDD2(MDDSpec& mdd, const Factory::Veci& x, int lb, int ub, std::set<int> rawValues) {
      mdd.append(x);
      ValueSet values(rawValues);
      auto d = mdd.makeConstraintDescriptor(x,"amongMDD");
      const int L = mdd.addState(d,0,x.size(),MinFun);
      const int U = mdd.addState(d,0,x.size(),MaxFun);
      const int Lup = mdd.addState(d,0,x.size(),MinFun);
      const int Uup = mdd.addState(d,0,x.size(),MaxFun);

      mdd.transitionDown(L,{L},[L,values] (auto& out,const auto& p,const auto& x, const auto& val,bool up) {
                                bool allMembers = true;
                                for(int v : val) {
                                   allMembers &= values.member(v);
                                   if (!allMembers) break;
                                }
                                out.set(L,p.at(L) + allMembers);
                             });
      mdd.transitionDown(U,{U},[U,values] (auto& out,const auto& p,const auto& x, const auto& val,bool up) {
                                bool oneMember = false;
                                for(int v : val) {
                                   oneMember = values.member(v);
                                   if (oneMember) break;
                                }
                                out.set(U,p.at(U) + oneMember);
                             });

      mdd.transitionUp(Lup,{Lup},[Lup,values] (auto& out,const auto& c,const auto& x, const auto& val,bool up) {
                                bool allMembers = true;
                                for(int v : val) {
                                   allMembers &= values.member(v);
                                   if (!allMembers) break;
                                }
                                out.set(Lup,c.at(Lup) + allMembers);
                             });
      mdd.transitionUp(Uup,{Uup},[Uup,values] (auto& out,const auto& p,const auto& x, const auto& val,bool up) {
                                bool oneMember = false;
                                for(int v : val) {
                                   oneMember = values.member(v);
                                   if (oneMember) break;
                                }
                                out.set(Uup,p.at(Uup) + oneMember);
                             });

      mdd.arcExist(d,[=] (const auto& p,const auto& c,var<int>::Ptr var, const auto& val, bool up) -> bool {
	  if (up) {
	    return ((p.at(U) + values.member(val) + c.at(Uup) >= lb) &&
		    (p.at(L) + values.member(val) + c.at(Lup) <= ub));
          } else
	    return (p.at(L) + values.member(val) <= ub);
      });

      mdd.splitOnLargest([lb,L,Lup,U](const auto& in) {
	  return -(double)(in.getState().at(U)-in.getState().at(L));
	});
   }
  
   void amongMDD2(MDDSpec& mdd, const Factory::Vecb& x, int lb, int ub, std::set<int> rawValues) {
      mdd.append(x);
      ValueSet values(rawValues);
      assert(rawValues.size()==1);
      int tv = *rawValues.cbegin();
      auto d = mdd.makeConstraintDescriptor(x,"amongMDD");
      const int L = mdd.addState(d,0,INT_MAX,MinFun);
      const int U = mdd.addState(d,0,INT_MAX,MaxFun);
      const int Lup = mdd.addState(d,0,INT_MAX,MinFun);
      const int Uup = mdd.addState(d,0,INT_MAX,MaxFun);

      mdd.transitionDown(L,{L},[L,tv] (auto& out,const auto& p,const auto& x, const auto& val,bool up) {
                                  bool allMembers = val.size() == 1 && val.singleton() == tv;
                                  out.setInt(L,p[L] + allMembers);
                               });
      mdd.transitionDown(U,{U},[U,tv] (auto& out,const auto& p,const auto& x, const auto& val,bool up) {
                                  bool oneMember = val.contains(tv);
                                  out.setInt(U,p[U] + oneMember);
                               });

      mdd.transitionUp(Lup,{Lup},[Lup,tv] (auto& out,const auto& c,const auto& x, const auto& val,bool up) {
                                    bool allMembers = val.size() == 1 && val.singleton() == tv;
                                    out.setInt(Lup,c[Lup] + allMembers);
                                 });
      mdd.transitionUp(Uup,{Uup},[Uup,tv] (auto& out,const auto& p,const auto& x, const auto& val,bool up) {
                                    bool oneMember = val.contains(tv);
                                    out.setInt(Uup,p[Uup] + oneMember);
                                 });

      mdd.arcExist(d,[tv,L,U,Lup,Uup,lb,ub] (const auto& p,const auto& c,var<int>::Ptr var, const auto& val, bool up) -> bool {
                        bool vinS = tv == val;//values.member(val);
                        if (up) {
                           return ((p[U] + vinS + c[Uup] >= lb) &&
                                   (p[L] + vinS + c[Lup] <= ub));
                        } else
                           return (p[L] + vinS <= ub);
      });

      // This function needs an 'up' flag?
      // mdd.nodeExist(d,[=](const auto& n) {
      // 	  return ( (n.at(L) + n.at(Lup) >= lb) &&
      // 		   (n.at(U) + n.at(Uup) <= ub));
      // 	});

      //mdd.splitOnLargest([](const auto& in) { return -(double)in.getNumParents();});

      mdd.splitOnLargest([lb,ub,L,Lup,U,Uup](const auto& in) {
      //   return -(double)in.getNumParents();
         return -((double)std::max(lb - (in.getState().at(L) + in.getState().at(Lup)),0) +
                  (double)std::max((in.getState().at(U) + in.getState().at(Uup)) - ub,0));
      //   return -(double)(in.getState().at(U) + in.getState().at(Uup) -
      //                    in.getState().at(L) - in.getState().at(Lup));

      //                      return -(double)(in.getState().at(U)-in.getState().at(L));
      //     //return (double)(std::max(lb - (in.getState().at(L) + in.getState().at(Lup)),0));
                            return 0.0;                            
                         });
      mdd.equivalenceClassValue([d,tv,lb,ub,L,Lup,U,Uup](const auto& p, const auto& c, var<int>::Ptr var, int val) -> int {
          //bool vinS = (d->inScope(var)) && tv == val;
          return (lb - (c.at(L) + c.at(Lup)) > 3) +
               2*(ub - (c.at(U) + c.at(Uup)) > 3);
      });
   }
}
