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
      mdd.arcExist(d,[minC,maxC,rem,tv,ub,lb] (const auto& p,const auto& c,var<int>::Ptr var, const auto& val,bool) -> bool {
                        bool vinS = tv == val;// values.member(val);
                        return (p[minC] + vinS <= ub) &&
                           ((p[maxC] + vinS +  p[rem] - 1) >= lb);
                     });         

      mdd.transitionDown(minC,[minC,tv] (auto& out,const auto& p,auto x, const auto& val,bool up) {
                                 bool allMembers = val.size()==1 && val.singleton() == tv;
                                 out.setInt(minC,p[minC] + allMembers);
                             });
      mdd.transitionDown(maxC,[maxC,tv] (auto& out,const auto& p,auto x, const auto& val,bool up) {
                                 bool oneMember = val.contains(tv);
                                out.setInt(maxC,p[maxC] + oneMember);
                             });
      mdd.transitionDown(rem,[rem] (auto& out,const auto& p,auto x,const auto& val,bool up) { out.setInt(rem,p[rem] - 1);});

      // mdd.addRelaxation(minC,[minC](auto& out,const auto& l,const auto& r) { out.setInt(minC,std::min(l[minC], r[minC]));});
      // mdd.addRelaxation(maxC,[maxC](auto& out,const auto& l,const auto& r) { out.setInt(maxC,std::max(l[maxC], r[maxC]));});
      // mdd.addRelaxation(rem ,[rem](auto& out,const auto& l,const auto& r)  { out.setInt(rem,std::max(l[rem],r[rem]));});

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
         mdd.arcExist(d,[minC,maxC,rem,tv,ub,lb] (const auto& p,const auto& c,var<int>::Ptr var, const auto& val,bool) -> bool {
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

      mdd.transitionDown(minC,[minC,values] (auto& out,const auto& p,auto x, const auto& val,bool up) {
                                bool allMembers = true;
                                for(int v : val) {
                                   allMembers &= values.member(v);
                                   if (!allMembers) break;
                                }
                                out.setInt(minC,p[minC] + allMembers);
                             });
      mdd.transitionDown(maxC,[maxC,values] (auto& out,const auto& p,auto x, const auto& val,bool up) {
                                bool oneMember = false;
                                for(int v : val) {
                                   oneMember = values.member(v);
                                   if (oneMember) break;
                                }
                                out.setInt(maxC,p[maxC] + oneMember);
                             });
      mdd.transitionDown(rem,[rem] (auto& out,const auto& p,auto x,const auto& val,bool up) { out.setInt(rem,p[rem] - 1);});

      // mdd.addRelaxation(minC,[minC](auto& out,const auto& l,const auto& r) { out.setInt(minC,std::min(l[minC], r[minC]));});
      // mdd.addRelaxation(maxC,[maxC](auto& out,const auto& l,const auto& r) { out.setInt(maxC,std::max(l[maxC], r[maxC]));});
      // mdd.addRelaxation(rem ,[rem](auto& out,const auto& l,const auto& r)  { out.setInt(rem,std::max(l[rem],r[rem]));});

      mdd.addSimilarity(minC,[minC](auto l,auto r) -> double { return abs(l[minC] - r[minC]); });
      mdd.addSimilarity(maxC,[maxC](auto l,auto r) -> double { return abs(l[maxC] - r[maxC]); });
      mdd.addSimilarity(rem ,[] (auto l,auto r) -> double { return 0; });
      mdd.splitOnLargest([](const auto& in) { return -(double)in.getNumParents();});
   }

   void amongMDD2(MDDSpec& mdd, const Factory::Veci& x, int lb, int ub, std::set<int> rawValues) {
      mdd.append(x);
      ValueSet values(rawValues);
      auto d = mdd.makeConstraintDescriptor(x,"amongMDD");
      const int L = mdd.addState(d,0,x.size());
      const int U = mdd.addState(d,0,x.size());
      const int Lup = mdd.addState(d,0,x.size());
      const int Uup = mdd.addState(d,0,x.size());

      mdd.transitionDown(L,[L,values] (auto& out,const auto& p,auto x, const auto& val,bool up) {
                                bool allMembers = true;
                                for(int v : val) {
                                   allMembers &= values.member(v);
                                   if (!allMembers) break;
                                }
                                out.set(L,p.at(L) + allMembers);
                             });
      mdd.transitionDown(U,[U,values] (auto& out,const auto& p,auto x, const auto& val,bool up) {
                                bool oneMember = false;
                                for(int v : val) {
                                   oneMember = values.member(v);
                                   if (oneMember) break;
                                }
                                out.set(U,p.at(U) + oneMember);
                             });

      mdd.transitionUp(Lup,[Lup,values] (auto& out,const auto& c,auto x, const auto& val,bool up) {
                                bool allMembers = true;
                                for(int v : val) {
                                   allMembers &= values.member(v);
                                   if (!allMembers) break;
                                }
                                out.set(Lup,c.at(Lup) + allMembers);
                             });
      mdd.transitionUp(Uup,[Uup,values] (auto& out,const auto& p,auto x, const auto& val,bool up) {
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

      // mdd.splitOnLargest([=](const auto& in) { return -(in[U]+in[Uup]-in[L]-in[Lup]);});
      // mdd.splitOnLargest([=](const auto& in) { return in[L];});

      
      mdd.addRelaxation(L,[L](auto& out,const auto& l,const auto& r) { out.set(L,std::min(l.at(L), r.at(L)));});
      mdd.addRelaxation(U,[U](auto& out,const auto& l,const auto& r) { out.set(U,std::max(l.at(U), r.at(U)));});
      mdd.addRelaxation(Lup,[Lup](auto& out,const auto& l,const auto& r) { out.set(Lup,std::min(l.at(Lup), r.at(Lup)));});
      mdd.addRelaxation(Uup,[Uup](auto& out,const auto& l,const auto& r) { out.set(Uup,std::max(l.at(Uup), r.at(Uup)));});

      mdd.addSimilarity(L,[L](auto l,auto r) -> double { return abs(l.at(L) - r.at(L)); });
      mdd.addSimilarity(U,[U](auto l,auto r) -> double { return abs(l.at(U) - r.at(U)); });
      mdd.addSimilarity(Lup,[Lup](auto l,auto r) -> double { return abs(l.at(Lup) - r.at(Lup)); });
      mdd.addSimilarity(Uup,[Uup](auto l,auto r) -> double { return abs(l.at(Uup) - r.at(Uup)); });
   }

  
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
      
      mdd.transitionDown(all,[minDom,all](auto& out,const auto& in,auto var,const auto& val,bool up) {
                               out.setProp(all,in);
                               if (val.size()==1)
                                  out.getBS(all).set(val.singleton() - minDom);
                               //out.setBS(all,in.getBS(all)).set(val - minDom);
                            });
      mdd.transitionDown(some,[minDom,some](auto& out,const auto& in,auto var,const auto& val,bool up) {
                                out.setProp(some,in);
                                MDDBSValue sv(out.getBS(some));
                                for(auto v : val)
                                   sv.set(v - minDom);
                                //out.setBS(some,in.getBS(some)).set(val - minDom);
                            });
      mdd.transitionDown(len,[len](auto& out,const auto& in,auto var,const auto& val,bool up) { out.set(len,in[len] + 1);});
      mdd.transitionUp(allu,[minDom,allu](auto& out,const auto& in,auto var,const auto& val,bool up) {
                               out.setProp(allu,in);
                               if (val.size()==1)
                                  out.getBS(allu).set(val.singleton() - minDom);
                            });
      mdd.transitionUp(someu,[minDom,someu](auto& out,const auto& in,auto var,const auto& val,bool up) {
                                out.setProp(someu,in);
                                MDDBSValue sv(out.getBS(someu));
                                for(auto v : val)
                                   sv.set(v - minDom);                                 
                             });
      
      mdd.addRelaxation(all,[all](auto& out,const auto& l,const auto& r)     {
                               out.getBS(all).setBinAND(l.getBS(all),r.getBS(all));
                            });
      mdd.addRelaxation(some,[some](auto& out,const auto& l,const auto& r)     {
                                out.getBS(some).setBinOR(l.getBS(some),r.getBS(some));
                            });
      mdd.addRelaxation(len,[len](auto& out,const auto& l,const auto& r)     { out.set(len,l[len]);});
      mdd.addRelaxation(allu,[allu](auto& out,const auto& l,const auto& r)     {
                               out.getBS(allu).setBinAND(l.getBS(allu),r.getBS(allu));
                            });
      mdd.addRelaxation(someu,[someu](auto& out,const auto& l,const auto& r)     {
                                out.getBS(someu).setBinOR(l.getBS(someu),r.getBS(someu));
                            });

      mdd.arcExist(d,[minDom,some,all,len,someu,allu,n](const auto& p,const auto& c,auto var,const auto& val,bool up) -> bool  {
                      MDDBSValue sbs = p.getBS(some);
                      const int ofs = val - minDom;
                      const bool notOk = p.getBS(all).getBit(ofs) || (sbs.getBit(ofs) && sbs.cardinality() == p[len]);
                      bool upNotOk = false,mixNotOk = false;
                      if (up) {
                         MDDBSValue subs = c.getBS(someu);
                         upNotOk = c.getBS(allu).getBit(ofs) || (subs.getBit(ofs) && subs.cardinality() == n - c[len]);
                         MDDBSValue both((char*)alloca(sizeof(unsigned long long)*subs.nbWords()),subs.nbWords());
                         both.setBinOR(subs,sbs).set(ofs);
                         mixNotOk = both.cardinality() < n;
                      }
                      return !notOk && !upNotOk && !mixNotOk;
                   });
      mdd.addSimilarity(all,[all](const auto& l,const auto& r) -> double {
                               MDDBSValue lv = l.getBS(all);
                               MDDBSValue tmp((char*)alloca(sizeof(char)*l.byteSize(all)),lv.nbWords());
                               tmp.setBinXOR(lv,r.getBS(all));
                               return tmp.cardinality();
                            });
      mdd.addSimilarity(some,[some](const auto& l,const auto& r) -> double {
                               MDDBSValue lv = l.getBS(some);
                               MDDBSValue tmp((char*)alloca(sizeof(char)*l.byteSize(some)),lv.nbWords());
                               tmp.setBinXOR(lv,r.getBS(some));
                               return tmp.cardinality();
                            });
      mdd.addSimilarity(len,[](const auto& l,const auto& r) -> double {
                               return 0;
                            });
   }

   void seqMDD(MDDSpec& spec,const Factory::Veci& vars, int len, int lb, int ub, std::set<int> rawValues)
   {
      int minFIdx = 0,minLIdx = len-1;
      int maxFIdx = len,maxLIdx = len*2-1;
      spec.append(vars);
      ValueSet values(rawValues);
      auto desc = spec.makeConstraintDescriptor(vars,"seqMDD");

      std::vector<int> ps = spec.addStates(desc,minFIdx,maxLIdx,SHRT_MAX,[maxLIdx,len,minLIdx] (int i) -> int {
         return (i - (i <= minLIdx ? minLIdx : (i >= len ? maxLIdx : 0)));
      });
      int p0 = ps[0];
      const int pminL = ps[minLIdx];
      const int pmaxF = ps[maxFIdx];
      const int pmin = ps[minFIdx];
      const int pmax = ps[maxLIdx];
      spec.arcExist(desc,[=] (const auto& p,const auto& c,auto x,int v,bool) -> bool {
                          bool inS = values.member(v);
                          int minv = p.at(pmax) - p.at(pmin) + inS;
                          return (p.at(p0) < 0 &&  minv >= lb && p.at(pminL) + inS               <= ub)
                             ||  (p.at(p0) >= 0 && minv >= lb && p.at(pminL) - p.at(pmaxF) + inS <= ub);
                       });
      
      spec.transitionDown(toDict(ps[minFIdx],
                                 ps[minLIdx]-1,
                                 [](int i) { return [i](auto& out,const auto& p,auto x,const auto& val,bool up) { out.set(i,p.at(i+1));};}));
      spec.transitionDown(toDict(ps[maxFIdx],
                                 ps[maxLIdx]-1,
                                 [](int i) { return [i](auto& out,const auto& p,auto x,const auto& val,bool up) { out.set(i,p.at(i+1));};}));
      spec.transitionDown(ps[minLIdx],[values,minL=ps[minLIdx]](auto& out,const auto& p,auto x,const auto& val,bool up) {
                                        bool allMembers = true;
                                        for(int v : val) {
                                           allMembers &= values.member(v);
                                           if (!allMembers) break;
                                        }
                                        out.set(minL,p.at(minL)+allMembers);
                                        //out.set(k,p.at(k)+values.member(v));
                                     });
      spec.transitionDown(ps[maxLIdx],[values,maxL=ps[maxLIdx]](auto& out,const auto& p,auto x,const auto& val,bool up) {
                                        bool oneMember = false;
                                        for(int v : val) {
                                           oneMember = values.member(v);
                                           if (oneMember) break;
                                        }
                                        out.set(maxL,p.at(maxL)+oneMember);
                                        //out.set(k,p.at(k)+values.member(v));
                                     });
      
      for(int i = minFIdx; i <= minLIdx; i++)
         spec.addRelaxation(ps[i],[p=ps[i]](auto& out,const auto& l,const auto& r) { out.set(p,std::min(l.at(p),r.at(p)));});
      for(int i = maxFIdx; i <= maxLIdx; i++)
         spec.addRelaxation(ps[i],[p=ps[i]](auto& out,const auto& l,const auto& r) { out.set(p,std::max(l.at(p),r.at(p)));});
      
      for(auto i : ps)
         spec.addSimilarity(i,[i](auto l,auto r)->double{return abs(l.at(i)- r.at(i));});
   }

   void seqMDD2(MDDSpec& spec,const Factory::Veci& vars, int len, int lb, int ub, std::set<int> rawValues)
   {
      int minFIdx = 0,minLIdx = len-1;
      int maxFIdx = len,maxLIdx = len*2-1;
      int nb = len*2;
      spec.append(vars);
      ValueSet values(rawValues);
      auto desc = spec.makeConstraintDescriptor(vars,"seqMDD");
      std::vector<int> ps(nb+1);
      for(int i = minFIdx;i < nb;i++)
         ps[i] = spec.addState(desc,0,len);       // init @ 0, largest value is length of window. 
      ps[nb] = spec.addState(desc,0,INT_MAX); // init @ 0, largest value is number of variables. 

      const int minL = ps[minLIdx];
      const int maxF = ps[maxFIdx];
      const int minF = ps[minFIdx];
      const int maxL = ps[maxLIdx];
      const int pnb  = ps[nb];

      spec.transitionDown(toDict(minF,minL-1,
                                 [](int i) { return [i](auto& out,const auto& p,auto x,const auto& val,bool up) { out.set(i,p.at(i+1));};}));
      spec.transitionDown(toDict(maxF,maxL-1,
                                 [](int i) { return [i](auto& out,const auto& p,auto x,const auto& val,bool up) { out.set(i,p.at(i+1));};}));

      spec.transitionDown(minL,[values,minL](auto& out,const auto& p,auto x,const auto& val,bool up) {
                                 bool allMembers = true;
                                 for(int v : val) {
                                    allMembers &= values.member(v);
                                    if (!allMembers) break;
                                 }
                                 out.set(minL,p.at(minL)+allMembers);
                              });
      spec.transitionDown(maxL,[values,maxL](auto& out,const auto& p,auto x,const auto& val,bool up) {
                                 bool oneMember = false;
                                 for(int v : val) {
                                    oneMember = values.member(v);
                                    if (oneMember) break;
                                 }
                                 out.set(maxL,p.at(maxL)+oneMember);
                              });
      spec.transitionDown(pnb,[pnb](auto& out,const auto& p,auto x,const auto& val,bool up) {
                                out.setInt(pnb,p[pnb]+1);
                             });

      spec.arcExist(desc,[=] (const auto& p,const auto& c,auto x,int v,bool) -> bool {
                          bool inS = values.member(v);
                          if (p.at(pnb) >= len - 1) {
                             bool c0 = p.at(maxL) + inS - p.at(minF) >= lb;
                             bool c1 = p.at(minL) + inS - p.at(maxF) <= ub;
                             return c0 && c1;
                          } else {
                             bool c0 = len - (p.at(pnb)+1) + p.at(maxL) + inS >= lb;
                             bool c1 = p.at(minL) + inS <= ub;
                             return c0 && c1;
                          }
                       });      
      
      for(int i = minFIdx; i <= minLIdx; i++)
         spec.addRelaxation(ps[i],[p=ps[i]](auto& out,const auto& l,const auto& r) { out.set(p,std::min(l.at(p),r.at(p)));});
      for(int i = maxFIdx; i <= maxLIdx; i++)
         spec.addRelaxation(ps[i],[p=ps[i]](auto& out,const auto& l,const auto& r) { out.set(p,std::max(l.at(p),r.at(p)));});
      spec.addRelaxation(pnb,[pnb](auto& out,const auto& l,const auto& r) { out.set(pnb,std::min(l.at(pnb),r.at(pnb)));});
   }

   void seqMDD3(MDDSpec& spec,const Factory::Veci& vars, int len, int lb, int ub, std::set<int> rawValues)
   {
      const int nbVars = (int)vars.size();

      // Indices of the properties that will be used.
      int YminIdx = 0,        YmaxIdx = 1;          // Minimum and maximum cumulative value at this state
      int AminFIdx = 2,       AminLIdx = 2+len-1;   // First and Last index of Amin array (minimum value among Ancestors)
      int AmaxFIdx = 2+len,   AmaxLIdx = 2+2*len-1; // First and Last index of Amax array (maximum value among Ancestors)
      int DminFIdx = 2+2*len, DminLIdx = 2+3*len-1; // First and Last index of Dmin array (maximum value among Descendants)
      int DmaxFIdx = 2+3*len, DmaxLIdx = 2+4*len-1; // First and Last index of Dmax array (maximum value among Descendants)
      int NIdx = 2+4*len;                        // Layer index (N-th variable)
      
      spec.append(vars);
      ValueSet values(rawValues);
      auto desc = spec.makeConstraintDescriptor(vars,"seqMDD");
      std::vector<int> ps(NIdx+1);

      ps[YminIdx] = spec.addState(desc, 0, nbVars,MinFun);
      ps[YmaxIdx] = spec.addState(desc, 0, nbVars,MaxFun);     
      for(int i = AminFIdx;i <= AminLIdx;i++)	ps[i] = spec.addState(desc, 0, nbVars,MinFun);
      for(int i = AmaxFIdx;i <= AmaxLIdx;i++)	ps[i] = spec.addState(desc, 0, nbVars,MaxFun);
      for(int i = DminFIdx;i <= DminLIdx;i++)	ps[i] = spec.addState(desc, 0, nbVars,MinFun);
      for(int i = DmaxFIdx;i <= DmaxLIdx;i++)	ps[i] = spec.addState(desc, 0, nbVars,MaxFun);
      ps[NIdx] = spec.addState(desc, 0, nbVars,MinFun);

      const int Ymin = ps[YminIdx];
      const int Ymax = ps[YmaxIdx];
      const int AminF = ps[AminFIdx];
      const int AminL = ps[AminLIdx];
      const int AmaxF = ps[AmaxFIdx];
      const int AmaxL = ps[AmaxLIdx];
      const int DminF = ps[DminFIdx];
      const int DminL = ps[DminLIdx];
      const int DmaxF = ps[DmaxFIdx];
      const int DmaxL = ps[DmaxLIdx];
      const int N = ps[NIdx];

      /*
      spec.transitionDown(Amin,[](auto& out,const auto& p,auto x,const auto&val, bool up) {
                                  MDDWindow ow = out.getWin(Amin);
                                  //ow = p.getWin(Amin) >> 1;
                                  ow.setWithShift(p.getWin(Amin),1);
                                  ow.setFirst(p.at(Ymin));
                               });
      */
      // down transitions
      spec.transitionDown(toDict(AminF+1,AminL,
                                 [](int i) { return [i](auto& out,const auto& p,auto x,const auto& val,bool up) { out.set(i,p.at(i-1));};}));
      spec.transitionDown(toDict(AmaxF+1,AmaxL,
                                 [](int i) { return [i](auto& out,const auto& p,auto x,const auto& val,bool up) { out.set(i,p.at(i-1));};}));

      spec.transitionDown(AminF,[AminF,Ymin](auto& out,const auto& p,auto x,const auto& val,bool up) { out.set(AminF,p.at(Ymin)); });
      spec.transitionDown(AmaxF,[AmaxF,Ymax](auto& out,const auto& p,auto x,const auto& val,bool up) { out.set(AmaxF,p.at(Ymax)); });

      spec.transitionDown(Ymin,[values,Ymin](auto& out,const auto& p,auto x,const auto& val,bool up) {
          bool hasMemberOutS = val.memberOutside(values);	  
	  int minVal = p.at(Ymin) + !hasMemberOutS;
	  if (up) 
             minVal = std::max(minVal, out.at(Ymin));	  
	  out.set(Ymin,minVal);
	});

      spec.transitionDown(Ymax,[values,Ymax](auto& out,const auto& p,auto x,const auto& val,bool up) {
          bool hasMemberInS = val.memberInside(values);
	  int maxVal = p.at(Ymax) + hasMemberInS;
	  if (up)
	    maxVal = std::min(maxVal, out.at(Ymax));
	  out.set(Ymax,maxVal);
	});

      spec.transitionDown(N,[N](auto& out,const auto& p,auto x,const auto& val,bool up) { out.set(N,p.at(N)+1); });

      // up transitions
      spec.transitionUp(toDict(DminF+1,DminL,
                               [](int i) { return [i](auto& out,const auto& c,auto x,const auto& val,bool up) { out.set(i,c.at(i-1));};}));
      spec.transitionUp(toDict(DmaxF+1,DmaxL,
                               [](int i) { return [i](auto& out,const auto& c,auto x,const auto& val,bool up) { out.set(i,c.at(i-1));};}));
      spec.transitionUp(DminF,[DminF,Ymin](auto& out,const auto& c,auto x,const auto& val,bool up) { out.set(DminF,c.at(Ymin)); });
      spec.transitionUp(DmaxF,[DmaxF,Ymax](auto& out,const auto& c,auto x,const auto& val,bool up) { out.set(DmaxF,c.at(Ymax)); });

      spec.transitionUp(Ymin,[Ymin,values](auto& out,const auto& c,auto x,const auto& val,bool up) {
                                bool hasMemberInS = val.memberInside(values);
                                int minVal = std::max(out.at(Ymin), c.at(Ymin) - hasMemberInS);
                                out.set(Ymin,minVal);
                             });

      spec.transitionUp(Ymax,[Ymax,values](auto& out,const auto& c,auto x,const auto& val,bool up) {
                                // std::cout << "entering Ymax Up at layer " << c.at(N) << " with values " << val;
                                bool hasMemberOutS = val.memberOutside(values);
                                int maxVal = std::min(out.at(Ymax), c.at(Ymax) - !hasMemberOutS);
                                out.set(Ymax,maxVal);
                             });

      spec.updateNode([=](auto& n) {
                         int minVal = n.at(Ymin);
                         int maxVal = n.at(Ymax);
                         if (n.at(N) >= len) {
                            minVal = std::max(lb + n.at(AminL),minVal);  // n.getWin(Amin).getLast()
                            maxVal = std::min(ub + n.at(AmaxL),maxVal);
                         }
                         if (n.at(N) <= nbVars - len) {
                            minVal = std::max(n.at(DminL) - ub,minVal);
                            maxVal = std::min(n.at(DmaxL) - lb,maxVal);
			 }
                         n.set(Ymin,minVal);
                         n.set(Ymax,maxVal);
                      });

      spec.nodeExist(desc,[=](const auto& p) {
	  return ( (p.at(Ymin) <= p.at(Ymax)) &&
		   (p.at(Ymax) >= 0) &&
		   (p.at(Ymax) <= p.at(N)) &&
		   (p.at(Ymin) >= 0) &&
		   (p.at(Ymin) <= p.at(N)) );
	});
      
      // arc definitions
      spec.arcExist(desc,[=] (const auto& p,const auto& c,auto x,int v,bool up) -> bool {
                            bool c0 = true,c1 = true,inS = values.member(v);
                            if (up) { // during the initial post, I do test arc existence and up isn't there yet.
                               c0 = (p.at(Ymin) + inS <= c.at(Ymax));
                               c1 = (p.at(Ymax) + inS >= c.at(Ymin));
                            }
                            return c0 && c1;
                         });      
      
      // relaxations
      // spec.addRelaxation(Ymin,[Ymin](auto& out,const auto& l,const auto& r) { out.set(Ymin,std::min(l.at(Ymin),r.at(Ymin)));});
      // spec.addRelaxation(Ymax,[Ymax](auto& out,const auto& l,const auto& r) { out.set(Ymax,std::max(l.at(Ymax),r.at(Ymax)));});
      // for(int i = AminFIdx; i <= AminLIdx; i++)
      //    spec.addRelaxation(ps[i],[p=ps[i]](auto& out,const auto& l,const auto& r) { out.set(p,std::min(l.at(p),r.at(p)));});
      // for(int i = AmaxFIdx; i <= AmaxLIdx; i++)
      //    spec.addRelaxation(ps[i],[p=ps[i]](auto& out,const auto& l,const auto& r) { out.set(p,std::max(l.at(p),r.at(p)));});
      // for(int i = DminFIdx; i <= DminLIdx; i++)
      //    spec.addRelaxation(ps[i],[p=ps[i]](auto& out,const auto& l,const auto& r) { out.set(p,std::min(l.at(p),r.at(p)));});
      // for(int i = DmaxFIdx; i <= DmaxLIdx; i++)
      //    spec.addRelaxation(ps[i],[p=ps[i]](auto& out,const auto& l,const auto& r) { out.set(p,std::max(l.at(p),r.at(p)));});
      // spec.addRelaxation(N,[N](auto& out,const auto& l,const auto& r) { out.set(N,std::min(l.at(N),r.at(N)));});
   }


  
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

      lambdaMap d = toDict(minFDom,maxLDom,ps,[dz,min,minLDom,ps] (int i,int pi) -> lambdaTrans {
             // LDM: TOFIX
              if (i <= minLDom)
                 return [=] (auto& out,const auto& p,auto x, const auto& val,bool up) { out.set(pi,p.at(pi) + ((val.singleton() - min) == i));};
              return [=] (auto& out,const auto& p,auto x, const auto& val,bool up)    { out.set(pi,p.at(pi) + ((val.singleton() - min) == (i - dz)));};
           });
      spec.transitionDown(d);

      for(ORInt i = minFDom; i <= minLDom; i++){
         int p = ps[i];
         spec.addRelaxation(p,[p](auto& out,auto l,auto r)  { out.set(p,std::min(l.at(p),r.at(p)));});
         spec.addSimilarity(p,[p](auto l,auto r)->double{return std::min(l.at(p),r.at(p));});
      }

      for(ORInt i = maxFDom; i <= maxLDom; i++){
         int p = ps[i];
         spec.addRelaxation(p,[p](auto& out,auto l,auto r) { out.set(p,std::max(l.at(p),r.at(p)));});
         spec.addSimilarity(p,[p](auto l,auto r)->double{return std::max(l.at(p),r.at(p));});
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
      
      spec.transitionDown(toDict(minFDom,minLDom, [min,ps] (int i) {
      	    return [=](auto& out,const auto& p,auto x,const auto& val,bool up) {
      	      int tmp = p.at(ps[i]);
      	      if (val.isSingleton() && (val.singleton() - min) == i) tmp++;
      	      out.set(ps[i], tmp);
      	    }; }));
      spec.transitionDown(toDict(maxFDom,maxLDom, [min,ps,maxFDom](int i) {
      	    return [=](auto& out,const auto& p,auto x,const auto& val,bool up) {
     	      out.set(ps[i], p.at(ps[i])+val.contains(i-maxFDom+min));
      	    }; }));

      spec.transitionUp(toDict(minFDomUp,minLDomUp, [min,ps,minFDomUp] (int i) {
      	    return [=](auto& out,const auto& c,auto x,const auto& val,bool up) {
	      out.set(ps[i], c.at(ps[i]) + (val.isSingleton() && (val.singleton() - min + minFDomUp == i)));
      	    }; }));
      spec.transitionUp(toDict(maxFDomUp,maxLDomUp, [min,ps,maxFDomUp](int i) {
      	    return [=](auto& out,const auto& c,auto x,const auto& val,bool up) {
	      out.set(ps[i], c.at(ps[i])+val.contains(i-maxFDomUp+min));
      	    }; }));

      
      // lambdaMap d = toDict(minFDom,maxLDom,ps,[dz,min,minLDom,maxFDom,ps] (int i,int pi) -> lambdaTrans {
      // 	  if (i <= minLDom)
      // 	    return [=] (auto& out,const auto& p,auto x, const auto& val,bool up) {
      // 	      out.set(pi,p.at(pi) + (val.isSingleton() && (val.singleton() - min) == i));};
      // 	  return [=] (auto& out,const auto& p,auto x, const auto& val,bool up)    {
      // 	    int tmp = p.at(ps[i]);
      // 	    for(int v : val) {
      // 	      if (i-maxFDom+min == v) {
      // 		tmp++;
      // 		break;
      // 	      }
      // 	    }
      // 	    out.set(pi,tmp);};
      //      });
      // spec.transitionDown(d);

      // lambdaMap dUp = toDict(minFDomUp,maxLDomUp,ps,[dz,min,minFDomUp,minLDomUp,maxFDom,ps] (int i,int pi) -> lambdaTrans {
      // 	  if (i <= minLDomUp)
      // 	    return [=] (auto& out,const auto& c,auto x, const auto& val,bool up) {
      // 	      out.set(pi,c.at(pi) + (val.isSingleton() && (i-minFDomUp+min==val.singleton())));};
      // 	  return [=] (auto& out,const auto& c,auto x, const auto& val,bool up)    {
      // 	    int tmp = c.at(ps[i]);
      // 	    for(int v : val) {
      // 	      if (i-maxFDom+min == v) {
      // 		tmp++;
      // 		break;
      // 	      }
      // 	    }
      // 	    out.set(pi,tmp);};
      // 	});
      // spec.transitionUp(dUp);

      
      for(ORInt i = minFDom; i <= minLDom; i++){
         int p = ps[i];
         spec.addRelaxation(p,[p](auto& out,auto l,auto r)  { out.set(p,std::min(l.at(p),r.at(p)));});
         spec.addSimilarity(p,[p](auto l,auto r)->double{return std::min(l.at(p),r.at(p));});
      }

      for(ORInt i = maxFDom; i <= maxLDom; i++){
         int p = ps[i];
         spec.addRelaxation(p,[p](auto& out,auto l,auto r) { out.set(p,std::max(l.at(p),r.at(p)));});
         spec.addSimilarity(p,[p](auto l,auto r)->double{return std::max(l.at(p),r.at(p));});
      }

      for(ORInt i = minFDomUp; i <= minLDomUp; i++){
	 int p = ps[i];
         spec.addRelaxation(p,[p](auto& out,auto l,auto r)  { out.set(p,std::min(l.at(p),r.at(p)));});
         spec.addSimilarity(p,[p](auto l,auto r)->double{return std::min(l.at(p),r.at(p));});
      }

      for(ORInt i = maxFDomUp; i <= maxLDomUp; i++){
         int p = ps[i];
         spec.addRelaxation(p,[p](auto& out,auto l,auto r) { out.set(p,std::max(l.at(p),r.at(p)));});
         spec.addSimilarity(p,[p](auto l,auto r)->double{return std::max(l.at(p),r.at(p));});
      }
}


  

  void sumMDD(MDDSpec& mdd, const Factory::Veci& vars, const std::vector<int>& array, int lb, int ub) {
      // Enforce
      //   sum(i, array[i]*vars[i]) >= lb and
      //   sum(i, array[i]*vars[i]) <= ub
      mdd.append(vars);

      // Create lower and upper bounds as proxy for bottom-up values.
      // At layer i, the proxy sums the minimum (resp. maximum) value
      // from layers i+1 through n.      
      int nbVars = (int)vars.size();
      std::vector<int> Lproxy(nbVars, 0);
      std::vector<int> Uproxy(nbVars, 0);
      Lproxy[nbVars-1] = 0;
      Uproxy[nbVars-1] = 0;
      for (int i=nbVars-2; i>=0; i--) {
	Lproxy[i] = Lproxy[i+1] + array[i+1]*vars[i+1]->min();
	Uproxy[i] = Uproxy[i+1] + array[i+1]*vars[i+1]->max();	
      }
     
      auto d = mdd.makeConstraintDescriptor(vars,"sumMDD");

      // Define the states: minimum and maximum weighted value (initialize at 0, maximum is INT_MAX (when negative values are allowed).
      const int minW = mdd.addState(d, 0, INT_MAX);
      const int maxW = mdd.addState(d, 0, INT_MAX);
      const int minWup = mdd.addState(d, 0, INT_MAX);
      const int maxWup = mdd.addState(d, 0, INT_MAX);

      // State 'len' is needed to capture the index i, to express array[i]*val when vars[i]=val.
      const int len  = mdd.addState(d, 0, vars.size());

      // The lower bound needs the bottom-up state information to be effective.
      mdd.arcExist(d,[=] (const auto& p, const auto& c, var<int>::Ptr var, const auto& val,bool upPass) -> bool {
	  if (upPass==true) {
	    return ((p[minW] + val*array[p[len]] + c[minWup] <= ub) &&
		    (p[maxW] + val*array[p[len]] + c[maxWup] >= lb));
	  } else {
	    return ((p[minW] + val*array[p[len]] + Lproxy[p[len]] <= ub) && 
		    (p[maxW] + val*array[p[len]] + Uproxy[p[len]] >= lb));
	  }
	});
	
      mdd.transitionDown(minW,[minW,array,len] (auto& out,const auto& p,auto var, const auto& val,bool up) {
                                int delta = std::numeric_limits<int>::max();
                                auto coef = array[p[len]];
                                for(int v : val)
                                   delta = std::min(delta,coef*v);
                                out.setInt(minW, p[minW] + delta);
                                //out.setInt(minW, p[minW] + array[p[len]]*val);
                             });
      mdd.transitionDown(maxW,[maxW,array,len] (auto& out,const auto& p,auto var, const auto& val,bool up) {
                                int delta = std::numeric_limits<int>::min();
                                auto coef = array[p[len]];
                                for(int v : val)
                                   delta = std::max(delta,coef*v);
                                out.setInt(maxW, p[maxW] + delta);
                                //out.setInt(maxW, p[maxW] + array[p[len]]*val);
                             });

      mdd.transitionUp(minWup,[minWup,array,len] (auto& out,const auto& in,auto var, const auto& val,bool up) {
                                  if (in[len] >= 1) {
                                     int delta = std::numeric_limits<int>::max();
                                     auto coef = array[in[len]-1];
                                     for(int v : val)
                                        delta = std::min(delta,coef*v);
                                     out.setInt(minWup, in[minWup] + delta);
                                  }
                               });
      mdd.transitionUp(maxWup,[maxWup,array,len] (auto& out,const auto& in,auto var, const auto& val,bool up) {
                                  if (in[len] >= 1) {
                                     int delta = std::numeric_limits<int>::min();
                                     auto coef = array[in[len]-1];
                                     for(int v : val)
                                        delta = std::max(delta,coef*v);
                                     out.setInt(maxWup, in[maxWup] + delta);
                                  }
                               });
      
      mdd.transitionDown(len, [len] (auto& out,const auto& p,auto var, const auto& val,bool up) {
                                out.setInt(len,  p[len] + 1);
                             });      

      mdd.addRelaxation(minW,[minW](auto& out,const auto& l,const auto& r) { out.setInt(minW,std::min(l[minW], r[minW]));});
      mdd.addRelaxation(maxW,[maxW](auto& out,const auto& l,const auto& r) { out.setInt(maxW,std::max(l[maxW], r[maxW]));});
      mdd.addRelaxation(minWup,[minWup](auto& out,const auto& l,const auto& r) { out.setInt(minWup,std::min(l[minWup], r[minWup]));});
      mdd.addRelaxation(maxWup,[maxWup](auto& out,const auto& l,const auto& r) { out.setInt(maxWup,std::max(l[maxWup], r[maxWup]));});
      mdd.addRelaxation(len, [len](auto& out,const auto& l,const auto& r)  { out.setInt(len,std::max(l[len],r[len]));});

      mdd.addSimilarity(minW,[minW](auto l,auto r) -> double { return abs(l.at(minW) - r.at(minW)); });
      mdd.addSimilarity(maxW,[maxW](auto l,auto r) -> double { return abs(l.at(maxW) - r.at(maxW)); });
      mdd.addSimilarity(minWup,[minWup](auto l,auto r) -> double { return abs(l.at(minWup) - r.at(minWup)); });
      mdd.addSimilarity(maxWup,[maxWup](auto l,auto r) -> double { return abs(l.at(maxWup) - r.at(maxWup)); });
      mdd.addSimilarity(len ,[] (auto l,auto r) -> double { return 0; }); 
  }

  void sumMDD(MDDSpec& mdd, const Factory::Veci& vars, const std::vector<int>& array, var<int>::Ptr z) {
      // Enforce MDD bounds consistency on
      //   sum(i, array[i]*vars[i]) == z 
      mdd.append(vars);
     
      // Create lower and upper bounds as proxy for bottom-up values.
      // At layer i, the proxy sums the minimum (resp. maximum) value
      // from layers i+1 through n.      
      int nbVars = (int)vars.size();
      std::vector<int> Lproxy(nbVars, 0);
      std::vector<int> Uproxy(nbVars, 0);
      Lproxy[nbVars-1] = 0;
      Uproxy[nbVars-1] = 0;
      for (int i=nbVars-2; i>=0; i--) {
	Lproxy[i] = Lproxy[i+1] + array[i+1]*vars[i+1]->min();
	Uproxy[i] = Uproxy[i+1] + array[i+1]*vars[i+1]->max();	
      }

      auto d = mdd.makeConstraintDescriptor(vars,"sumMDD");

      // Define the states
      const int minW = mdd.addState(d, 0, INT_MAX);
      const int maxW = mdd.addState(d, 0, INT_MAX);
      const int minWup = mdd.addState(d, 0, INT_MAX);
      const int maxWup = mdd.addState(d, 0, INT_MAX);

      // State 'len' is needed to capture the index i, to express array[i]*val when vars[i]=val.
      const int len  = mdd.addState(d, 0, vars.size());

      mdd.arcExist(d,[=] (const auto& p, const auto& c, var<int>::Ptr var, const auto& val,bool upPass) -> bool {
	  if (upPass==true) {
	    return ((p[minW] + val*array[p[len]] + c[minWup] <= z->max()) &&
		    (p[maxW] + val*array[p[len]] + c[maxWup] >= z->min()));
	  } else {
	    return ((p[minW] + val*array[p[len]] + Lproxy[p[len]] <= z->max()) && 
		    (p[maxW] + val*array[p[len]] + Uproxy[p[len]] >= z->min()));
	  }
	});

      
      mdd.transitionDown(minW,[minW,array,len] (auto& out,const auto& p,auto var, const auto& val,bool up) {
                                int delta = std::numeric_limits<int>::max();
                                auto coef = array[p[len]];
                                for(int v : val)
                                   delta = std::min(delta,coef * v);
                                out.setInt(minW,p[minW] + delta);
                                //out.set(minW, p[minW] + array[p[len]]*val);
                             });
      mdd.transitionDown(maxW,[maxW,array,len] (auto& out,const auto& p,auto var, const auto& val,bool up) {
                                int delta = std::numeric_limits<int>::min();
                                auto coef = array[p[len]];
                                for(int v : val)
                                   delta = std::max(delta,coef*v);
                                out.setInt(maxW, p[maxW] + delta);
                                //out.set(maxW, p[maxW] + array[p[len]]*val);
                             });

      mdd.transitionUp(minWup,[minWup,array,len] (auto& out,const auto& in,auto var, const auto& val,bool up) {
                                 if (in[len] >= 1) {
                                    int delta = std::numeric_limits<int>::max();
                                    auto coef = array[in[len]-1];
                                    for(int v : val)
                                       delta = std::min(delta,coef*v);
                                    out.setInt(minWup, in[minWup] + delta);
                                    //out.set(minWup, in[minWup] + array[in[len]-1]*val);
                                 }
                              });
      mdd.transitionUp(maxWup,[maxWup,array,len] (auto& out,const auto& in,auto var, const auto& val,bool up) {
                                 if (in[len] >= 1) {
                                    int delta = std::numeric_limits<int>::min();
                                    auto coef = array[in[len]-1];
                                    for(int v : val)
                                       delta = std::max(delta,coef*v);
                                    out.setInt(maxWup, in[maxWup] + delta);
                                    //out.set(maxWup, in[maxWup] + array[in[len]-1]*val);
                                 }
                              });

      
      mdd.transitionDown(len, [len](auto& out,const auto& p,auto var, const auto& val,bool up) {
                                out.setInt(len,p[len] + 1);
                             });      

      mdd.addRelaxation(minW,[minW](auto& out,const auto& l,const auto& r) { out.setInt(minW,std::min(l.at(minW), r.at(minW)));});
      mdd.addRelaxation(maxW,[maxW](auto& out,const auto& l,const auto& r) { out.setInt(maxW,std::max(l.at(maxW), r.at(maxW)));});
      mdd.addRelaxation(minWup,[minWup](auto& out,const auto& l,const auto& r) { out.setInt(minWup,std::min(l[minWup], r[minWup]));});
      mdd.addRelaxation(maxWup,[maxWup](auto& out,const auto& l,const auto& r) { out.setInt(maxWup,std::max(l[maxWup], r[maxWup]));});
      mdd.addRelaxation(len, [len](auto& out,const auto& l,const auto& r)  { out.setInt(len,std::max(l[len],r[len]));});

      mdd.addSimilarity(minW,[minW](auto l,auto r) -> double { return abs(l.at(minW) - r.at(minW)); });
      mdd.addSimilarity(maxW,[maxW](auto l,auto r) -> double { return abs(l.at(maxW) - r.at(maxW)); });
      mdd.addSimilarity(minWup,[minWup](auto l,auto r) -> double { return abs(l[minWup] - r[minWup]); });
      mdd.addSimilarity(maxWup,[maxWup](auto l,auto r) -> double { return abs(l[maxWup] - r[maxWup]); });
      mdd.addSimilarity(len ,[] (auto l,auto r) -> double { return 0; }); 
  }  

  void sumMDD(MDDSpec& mdd, const Factory::Veci& vars, const std::vector<std::vector<int>>& matrix, var<int>::Ptr z) {
      // Enforce MDD bounds consistency on
      //   sum(i, matrix[i][vars[i]]) == z 
      mdd.append(vars);

      // Create lower and upper bounds as proxy for bottom-up values.
      // At layer i, the proxy sums the minimum (resp. maximum) value
      // from layers i+1 through n.      
      int nbVars = (int)vars.size();
      std::vector<int> Lproxy(nbVars, 0);
      std::vector<int> Uproxy(nbVars, 0);
      Lproxy[nbVars-1] = 0;
      Uproxy[nbVars-1] = 0;
      for (int i=nbVars-2; i>=0; i--) {
	int tmpMin = INT_MAX;
	int tmpMax = -INT_MAX;
	for (int j=vars[i+1]->min(); j<=vars[i+1]->max(); j++) {
	  if (vars[i]->contains(j)) {
	    if (matrix[i+1][j] < tmpMin) { tmpMin = matrix[i+1][j]; }
	    if (matrix[i+1][j] > tmpMax) { tmpMax = matrix[i+1][j]; }
	  }
	}
	Lproxy[i] = Lproxy[i+1] + tmpMin;
	Uproxy[i] = Uproxy[i+1] + tmpMax;
      }
      
      auto d = mdd.makeConstraintDescriptor(vars,"sumMDD");

      // Define the states
      const int minW = mdd.addState(d, 0, INT_MAX);
      const int maxW = mdd.addState(d, 0, INT_MAX);
      const int minWup = mdd.addState(d, 0, INT_MAX);
      const int maxWup = mdd.addState(d, 0, INT_MAX);

      // State 'len' is needed to capture the index i, to express matrix[i][vars[i]]
      const int len  = mdd.addState(d, 0, vars.size());

      mdd.arcExist(d,[=] (const auto& p, const auto& c, var<int>::Ptr var, const auto& val,bool upPass) -> bool {
                      const int mlv = matrix[p[len]][val];
                      if (upPass==true) {
                         return ((p[minW] + mlv + c[minWup] <= z->max()) &&
                                 (p[maxW] + mlv + c[maxWup] >= z->min()));
                      } else {
                         return ((p[minW] + mlv + Lproxy[p[len]] <= z->max()) && 
                                 (p[maxW] + mlv + Uproxy[p[len]] >= z->min()));
                      }
                   });
      
      mdd.transitionDown(minW,[minW,matrix,len] (auto& out,const auto& p,auto var, const auto& val,bool up) {
                                int delta = std::numeric_limits<int>::max();
                                const auto& row = matrix[p[len]];
                                for(int v : val)
                                   delta = std::min(delta,row[v]);
                                out.setInt(minW,p[minW] + delta);
                                //out.setInt(minW, p[minW] + matrix[p[len]][val]);
                             });
      mdd.transitionDown(maxW,[maxW,matrix,len] (auto& out,const auto& p,auto var, const auto& val,bool up) {
                                int delta = std::numeric_limits<int>::min();
                                const auto& row = matrix[p[len]];
                                for(int v : val)
                                   delta = std::max(delta,row[v]);
                                out.setInt(maxW,p[maxW] + delta);
                                //out.setInt(maxW, p[maxW] + matrix[p[len]][val]);
                             });

      mdd.transitionUp(minWup,[minWup,matrix,len] (auto& out,const auto& in,auto var, const auto& val,bool up) {
                                  if (in[len] >= 1) {
                                     int delta = std::numeric_limits<int>::max();
                                     const auto& row = matrix[in[len]-1];
                                     for(int v : val)
                                        delta = std::min(delta,row[v]);
                                     out.setInt(minWup, in[minWup] + delta);
                                     //out.setInt(minWup, in[minWup] + matrix[in[len]-1][val]);
                                  }
                               });
      mdd.transitionUp(maxWup,[maxWup,matrix,len] (auto& out,const auto& in,auto var, const auto& val,bool up) {
                                  if (in.at(len) >= 1) {
                                     int delta = std::numeric_limits<int>::min();
                                     const auto& row = matrix[in[len]-1];
                                     for(int v : val)
                                        delta = std::max(delta,row[v]);
                                     out.setInt(maxWup, in[maxWup] + delta);
                                     //out.setInt(maxWup, in[maxWup] + matrix[in[len]-1][val]);
                                  }
                               });
      
      mdd.transitionDown(len, [len](auto& out,const auto& p,auto var, const auto& val,bool up) {
                                out.setInt(len,  p[len] + 1);
                             });      

      mdd.addRelaxation(minW,[minW](auto& out,const auto& l,const auto& r) { out.setInt(minW,std::min(l[minW], r[minW]));});
      mdd.addRelaxation(maxW,[maxW](auto& out,const auto& l,const auto& r) { out.setInt(maxW,std::max(l[maxW], r[maxW]));});
      mdd.addRelaxation(minWup,[minWup](auto& out,const auto& l,const auto& r) { out.setInt(minWup,std::min(l[minWup], r[minWup]));});
      mdd.addRelaxation(maxWup,[maxWup](auto& out,const auto& l,const auto& r) { out.setInt(maxWup,std::max(l[maxWup], r[maxWup]));});
      mdd.addRelaxation(len, [len](auto& out,const auto& l,const auto& r)  { out.setInt(len,std::max(l[len],r[len]));});

      mdd.onFixpoint([z,minW,maxW](const auto& sink) {
                        z->updateBounds(sink.at(minW),sink.at(maxW));
                     });

      mdd.splitOnLargest([minW](const auto& in) { return in.getState()[minW];});

      mdd.addSimilarity(minW,[minW](auto l,auto r) -> double { return abs(l.at(minW) - r.at(minW)); });
      mdd.addSimilarity(maxW,[maxW](auto l,auto r) -> double { return abs(l.at(maxW) - r.at(maxW)); });
      mdd.addSimilarity(minWup,[minWup](auto l,auto r) -> double { return abs(l[minWup] - r[minWup]); });
      mdd.addSimilarity(maxWup,[maxWup](auto l,auto r) -> double { return abs(l[maxWup] - r[maxWup]); });
      mdd.addSimilarity(len ,[] (auto l,auto r) -> double { return 0; }); 
  }
}
