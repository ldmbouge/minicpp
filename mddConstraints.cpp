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
#include <limits.h>

namespace Factory {
   void amongMDD(MDDSpec& mdd, const Factory::Veci& x, int lb, int ub, std::set<int> rawValues) {
      mdd.append(x);
      ValueSet values(rawValues);
      auto& d = mdd.makeConstraintDescriptor(x,"amongMDD");
      const int minC = mdd.addState(d,0,x.size());
      const int maxC = mdd.addState(d,0,x.size());
      const int rem  = mdd.addState(d,(int)x.size(),x.size());

      mdd.addArc(d,[=] (const auto& p,const auto& c,var<int>::Ptr var, int val,bool) -> bool {
         return (p.at(minC) + values.member(val) <= ub) &&
                ((p.at(maxC) + values.member(val) +  p.at(rem) - 1) >= lb);
      });

      mdd.addTransition(minC,[minC,values] (auto& out,const auto& p,auto x, int v) { out.set(minC,p.at(minC) + values.member(v));});
      mdd.addTransition(maxC,[maxC,values] (auto& out,const auto& p,auto x, int v) { out.set(maxC,p.at(maxC) + values.member(v));});
      mdd.addTransition(rem,[rem] (auto& out,const auto& p,auto x,int v)           { out.set(rem,p.at(rem) - 1);});

      mdd.addRelaxation(minC,[minC](auto& out,const auto& l,const auto& r) { out.set(minC,std::min(l.at(minC), r.at(minC)));});
      mdd.addRelaxation(maxC,[maxC](auto& out,const auto& l,const auto& r) { out.set(maxC,std::max(l.at(maxC), r.at(maxC)));});
      mdd.addRelaxation(rem ,[rem](auto& out,const auto& l,const auto& r)  { out.set(rem,l.at(rem));});

      mdd.addSimilarity(minC,[minC](auto l,auto r) -> double { return abs(l.at(minC) - r.at(minC)); });
      mdd.addSimilarity(maxC,[maxC](auto l,auto r) -> double { return abs(l.at(maxC) - r.at(maxC)); });
      mdd.addSimilarity(rem ,[] (auto l,auto r) -> double { return 0; });
   }

   void allDiffMDD(MDDSpec& mdd, const Factory::Veci& vars)
   {
      mdd.append(vars);
      auto& d = mdd.makeConstraintDescriptor(vars,"allDiffMdd");
      auto udom = domRange(vars);
      int minDom = udom.first;
      const int n    = (int)vars.size();
      const int all  = mdd.addBSState(d,udom.second - udom.first + 1,0);
      const int some = mdd.addBSState(d,udom.second - udom.first + 1,0);
      const int len  = mdd.addState(d,0,vars.size());
      const int allu = mdd.addBSStateUp(d,udom.second - udom.first + 1,0);
      const int someu = mdd.addBSStateUp(d,udom.second - udom.first + 1,0);
      
      mdd.addTransition(all,[minDom,all](auto& out,const auto& in,auto var,int val) {
                               out.setBS(all,in.getBS(all)).set(val - minDom);
                            });
      mdd.addTransition(some,[minDom,some](auto& out,const auto& in,auto var,int val) {
                                out.setBS(some,in.getBS(some)).set(val - minDom);
                            });
      mdd.addTransition(len,[len,d](auto& out,const auto& in,auto var,int val) { out.set(len,in.at(len) + 1);});
      mdd.addTransition(allu,[minDom,allu](auto& out,const auto& in,auto var,int val) {
                                  out.setBS(allu,in.getBS(allu)).set(val - minDom);
                               });
      mdd.addTransition(someu,[minDom,someu](auto& out,const auto& in,auto var,int val) {
                                  out.setBS(someu,in.getBS(someu)).set(val - minDom);
                               });
      
      mdd.addRelaxation(all,[all](auto& out,const auto& l,const auto& r)     {
                               out.getBS(all).setBinAND(l.getBS(all),r.getBS(all));
                            });
      mdd.addRelaxation(some,[some](auto& out,const auto& l,const auto& r)     {
                                out.getBS(some).setBinOR(l.getBS(some),r.getBS(some));
                            });
      mdd.addRelaxation(len,[len](auto& out,const auto& l,const auto& r)     { out.set(len,l.at(len));});
      mdd.addRelaxation(allu,[allu](auto& out,const auto& l,const auto& r)     {
                               out.getBS(allu).setBinAND(l.getBS(allu),r.getBS(allu));
                            });
      mdd.addRelaxation(someu,[someu](auto& out,const auto& l,const auto& r)     {
                                out.getBS(someu).setBinOR(l.getBS(someu),r.getBS(someu));
                            });

      mdd.addArc(d,[minDom,some,all,len,someu,allu,n](const auto& p,const auto& c,auto var,int val,bool up) -> bool  {
                      MDDBSValue sbs = p.getBS(some);
                      const int ofs = val - minDom;
                      const bool notOk = p.getBS(all).getBit(ofs) || (sbs.getBit(ofs) && sbs.cardinality() == p.at(len));
                      bool upNotOk = false,mixNotOk = false;
                      if (up) {
                         MDDBSValue subs = c.getBS(someu);
                         upNotOk = c.getBS(allu).getBit(ofs) || (subs.getBit(ofs) && subs.cardinality() == n - c.at(len));
                         MDDBSValue both((char*)alloca(sizeof(unsigned long long)*subs.nbWords()),subs.nbWords(),subs.bitLen());
                         both.setBinOR(subs,sbs).set(ofs);
                         mixNotOk = both.cardinality() < n;
                      }
                      return !notOk && !upNotOk && !mixNotOk;
                   });
      mdd.addSimilarity(all,[all](const auto& l,const auto& r) -> double {
                               MDDBSValue lv = l.getBS(all);
                               MDDBSValue tmp((char*)alloca(sizeof(char)*l.byteSize(all)),lv.nbWords(),lv.bitLen());
                               tmp.setBinXOR(lv,r.getBS(all));
                               return tmp.cardinality();
                            });
      mdd.addSimilarity(some,[some](const auto& l,const auto& r) -> double {
                               MDDBSValue lv = l.getBS(some);
                               MDDBSValue tmp((char*)alloca(sizeof(char)*l.byteSize(some)),lv.nbWords(),lv.bitLen());
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
      auto& desc = spec.makeConstraintDescriptor(vars,"seqMDD");

      std::vector<int> ps = spec.addStates(desc,minFIdx,maxLIdx,SHRT_MAX,[maxLIdx,len,minLIdx] (int i) -> int {
         return (i - (i <= minLIdx ? minLIdx : (i >= len ? maxLIdx : 0)));
      });
      int p0 = ps[0];
      const int pminL = ps[minLIdx];
      const int pmaxF = ps[maxFIdx];
      const int pmin = ps[minFIdx];
      const int pmax = ps[maxLIdx];
      spec.addArc(desc,[=] (const auto& p,const auto& c,auto x,int v,bool) -> bool {
                          bool inS = values.member(v);
                          int minv = p.at(pmax) - p.at(pmin) + inS;
                          return (p.at(p0) < 0 &&  minv >= lb && p.at(pminL) + inS               <= ub)
                             ||  (p.at(p0) >= 0 && minv >= lb && p.at(pminL) - p.at(pmaxF) + inS <= ub);
                       });
      
      spec.addTransitions(toDict(ps[minFIdx],
                                 ps[minLIdx]-1,
                                 [](int i) { return [i](auto& out,const auto& p,auto x,int v) { out.set(i,p.at(i+1));};}));
      spec.addTransitions(toDict(ps[maxFIdx],
                                 ps[maxLIdx]-1,
                                 [](int i) { return [i](auto& out,const auto& p,auto x,int v) { out.set(i,p.at(i+1));};}));
      spec.addTransition(ps[minLIdx],[values,
                                      k=ps[minLIdx]](auto& out,const auto& p,auto x,int v) { out.set(k,p.at(k)+values.member(v));});
      spec.addTransition(ps[maxLIdx],[values,
                                      k=ps[maxLIdx]](auto& out,const auto& p,auto x,int v) { out.set(k,p.at(k)+values.member(v));});
      
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
      auto& desc = spec.makeConstraintDescriptor(vars,"seqMDD");
      std::vector<int> ps(nb+1);
      for(int i = minFIdx;i < nb;i++)
         ps[i] = spec.addState(desc,0,len);       // init @ 0, largest value is length of window. 
      ps[nb] = spec.addState(desc,0,INT_MAX); // init @ 0, largest value is number of variables. 

      const int minL = ps[minLIdx];
      const int maxF = ps[maxFIdx];
      const int minF = ps[minFIdx];
      const int maxL = ps[maxLIdx];
      const int pnb  = ps[nb];

      spec.addTransitions(toDict(minF,minL-1,
                                 [](int i) { return [i](auto& out,const auto& p,auto x,int v) { out.set(i,p.at(i+1));};}));
      spec.addTransitions(toDict(maxF,maxL-1,
                                 [](int i) { return [i](auto& out,const auto& p,auto x,int v) { out.set(i,p.at(i+1));};}));
      spec.addTransition(minL,[values,minL](auto& out,const auto& p,auto x,int v) { out.set(minL,p.at(minL)+values.member(v));});
      spec.addTransition(maxL,[values,maxL](auto& out,const auto& p,auto x,int v) { out.set(maxL,p.at(maxL)+values.member(v));});
      spec.addTransition(pnb,[pnb](auto& out,const auto& p,auto x,int v) {
                                out.set(pnb,p.at(pnb)+1);
                             });

      spec.addArc(desc,[=] (const auto& p,const auto& c,auto x,int v,bool) -> bool {
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
     // This version tests whether we can update the state variables as well (eqns (6) and (8) in JAIR paper)
      int minFIdx = 0,minLIdx = len-1;
      int maxFIdx = len,maxLIdx = len*2-1;
      int nb = len*2;
      spec.append(vars);
      ValueSet values(rawValues);
      auto& desc = spec.makeConstraintDescriptor(vars,"seqMDD");
      std::vector<int> ps(nb+1);
      for(int i = minFIdx;i < nb;i++)
         ps[i] = spec.addState(desc,0,len);       // init @ 0, largest value is length of window. 
      ps[nb] = spec.addState(desc,0,INT_MAX); // init @ 0, largest value is number of variables. 

      const int minL = ps[minLIdx];
      const int maxF = ps[maxFIdx];
      const int minF = ps[minFIdx];
      const int maxL = ps[maxLIdx];
      const int pnb  = ps[nb];

      spec.addTransitions(toDict(minF,minL-1,
                                 [](int i) { return [i](auto& out,const auto& p,auto x,int v) { out.set(i,p.at(i+1));};}));
      spec.addTransitions(toDict(maxF,maxL-1,
                                 [](int i) { return [i](auto& out,const auto& p,auto x,int v) { out.set(i,p.at(i+1));};}));
      spec.addTransition(minL,[values,minL](auto& out,const auto& p,auto x,int v) { out.set(minL,p.at(minL)+values.member(v));});
      spec.addTransition(maxL,[values,maxL](auto& out,const auto& p,auto x,int v) { out.set(maxL,p.at(maxL)+values.member(v));});
      spec.addTransition(pnb,[pnb](auto& out,const auto& p,auto x,int v) {
                                out.set(pnb,p.at(pnb)+1);
                             });

      spec.addArc(desc,[=] (const auto& p,const auto& c,auto x,int v,bool) -> bool {
                          bool inS = values.member(v);
                          if (p.at(pnb) >= len - 1) {
                             bool c0 = p.at(maxL) + inS - p.at(minF) >= lb;
                             bool c1 = p.at(minL) + inS - p.at(maxF) <= ub;
                             bool c2 = p.at(minL) + inS >= p.at(minF) + lb;
                             bool c3 = p.at(maxL) + inS <= p.at(maxF) + ub;
                             return c0 && c1 && c2 && c3;
                          } else {
                             bool c0 = len - (p.at(pnb)+1) + p.at(maxL) + inS >= lb;
                             bool c1 = p.at(minL) + inS <= ub;
                             bool c2 = len - (p.at(pnb)+1) + p.at(minL) + inS >= lb;
                             bool c3 = p.at(maxL) + inS <= ub;
                             return c0 && c1 && c2 && c3;
                          }
                       });      
      
      for(int i = minFIdx; i < minLIdx; i++)
         spec.addRelaxation(ps[i],[p=ps[i]](auto& out,const auto& l,const auto& r) { out.set(p,std::min(l.at(p),r.at(p)));});

      spec.addRelaxation(minL,[minL,minF,lb,len,pnb](auto& out,const auto& l,const auto& r) {
	  int minVal = std::min(l.at(minL),r.at(minL));
	  if (l.at(pnb) >= len - 1) {
	    minVal = std::max(minVal, lb + std::min(l.at(minF),r.at(minF)));
	  }
	  else {
	    minVal = std::max(minVal, lb - (len - (l.at(pnb)+1)) + std::min(l.at(minF),r.at(minF)));
	  }
	  out.set(minL, minVal);
	});
      
      for(int i = maxFIdx; i < maxLIdx; i++)
         spec.addRelaxation(ps[i],[p=ps[i]](auto& out,const auto& l,const auto& r) { out.set(p,std::max(l.at(p),r.at(p)));});
      spec.addRelaxation(maxL,[maxL,maxF,ub](auto& out,const auto& l,const auto& r) {
	  int maxVal = std::max(l.at(maxL),r.at(maxL));
	  // add len-test for bottom-up information
	  maxVal = std::min(maxVal, ub + std::max(l.at(maxF),r.at(maxF)));
	  out.set(maxL, maxVal);
	});

      spec.addRelaxation(pnb,[pnb](auto& out,const auto& l,const auto& r) { out.set(pnb,std::min(l.at(pnb),r.at(pnb)));});
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
      auto& desc = spec.makeConstraintDescriptor(vars,"gccMDD");

      std::vector<int> ps = spec.addStates(desc,minFDom, maxLDom,sz,[] (int i) -> int { return 0; });

      spec.addArc(desc,[=](const auto& p,const auto& c,auto x,int v,bool)->bool{
                          return p.at(ps[v-min]) < values[v];
                       });

      lambdaMap d = toDict(minFDom,maxLDom,ps,[dz,min,minLDom,ps] (int i,int pi) -> lambdaTrans {
              if (i <= minLDom)
                 return [=] (auto& out,const auto& p,auto x, int v) { out.set(pi,p.at(pi) + ((v - min) == i));};
              return [=] (auto& out,const auto& p,auto x, int v)    { out.set(pi,p.at(pi) + ((v - min) == (i - dz)));};
           });
      spec.addTransitions(d);

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


  void sumMDD(MDDSpec& mdd, const Factory::Veci& vars, const std::vector<int>& array, int lb, int ub) {
      // Enforce
      //   sum(i, array[i]*vars[i]) >= lb and
      //   sum(i, array[i]*vars[i]) <= ub
      mdd.append(vars);

      // Create lower and upper bounds as proxy for bottom-up values.
      // At layer i, the proxy sums the minimum (resp. maximum) value
      // from layers i+1 through n.      
      int nbVars = vars.size();
      std::vector<int> Lproxy(nbVars, 0);
      std::vector<int> Uproxy(nbVars, 0);
      Lproxy[nbVars-1] = 0;
      Uproxy[nbVars-1] = 0;
      for (int i=nbVars-2; i>=0; i--) {
	Lproxy[i] = Lproxy[i+1] + array[i+1]*vars[i+1]->min();
	Uproxy[i] = Uproxy[i+1] + array[i+1]*vars[i+1]->max();	
      }
     
      auto& d = mdd.makeConstraintDescriptor(vars,"sumMDD");

      // Define the states: minimum and maximum weighted value (initialize at 0, maximum is INT_MAX (when negative values are allowed).
      const int minW = mdd.addState(d, 0, INT_MAX);
      const int maxW = mdd.addState(d, 0, INT_MAX);
      const int minWup = mdd.addStateUp(d, 0, INT_MAX);
      const int maxWup = mdd.addStateUp(d, 0, INT_MAX);

      // State 'len' is needed to capture the index i, to express array[i]*val when vars[i]=val.
      const int len  = mdd.addState(d, 0, vars.size());

      // The lower bound needs the bottom-up state information to be effective.
      mdd.addArc(d,[=] (const auto& p, const auto& c, var<int>::Ptr var, int val,bool upPass) -> bool {
	  if (upPass==true) {
	    return ((p.at(minW) + val*array[p.at(len)] + c.at(minWup) <= ub) &&
		    (p.at(maxW) + val*array[p.at(len)] + c.at(maxWup) >= lb));
	  } else {
	    return ((p.at(minW) + val*array[p.at(len)] + Lproxy[p.at(len)] <= ub) && 
		    (p.at(maxW) + val*array[p.at(len)] + Uproxy[p.at(len)] >= lb));
	  }
	});
	
      mdd.addTransition(minW,[minW,array,len] (auto& out,const auto& p,auto var, int val) {
	  out.set(minW, p.at(minW) + array[p.at(len)]*val);});
      mdd.addTransition(maxW,[maxW,array,len] (auto& out,const auto& p,auto var, int val) {
	  out.set(maxW, p.at(maxW) + array[p.at(len)]*val);});

      mdd.addTransition(minWup,[minWup,array,len] (auto& out,const auto& in,auto var, int val) {
	  if (in.at(len) >= 1) {
	    out.set(minWup, in.at(minWup) + array[in.at(len)-1]*val);
	  }
	});
      mdd.addTransition(maxWup,[maxWup,array,len] (auto& out,const auto& in,auto var, int val) {
	  if (in.at(len) >= 1) {
	    out.set(maxWup, in.at(maxWup) + array[in.at(len)-1]*val);
	  }
	});
      
      mdd.addTransition(len, [len] (auto& out,const auto& p,auto var, int val) {
                                out.set(len,  p.at(len) + 1);
                             });      

      mdd.addRelaxation(minW,[minW](auto& out,const auto& l,const auto& r) { out.set(minW,std::min(l.at(minW), r.at(minW)));});
      mdd.addRelaxation(maxW,[maxW](auto& out,const auto& l,const auto& r) { out.set(maxW,std::max(l.at(maxW), r.at(maxW)));});
      mdd.addRelaxation(minWup,[minWup](auto& out,const auto& l,const auto& r) { out.set(minWup,std::min(l.at(minWup), r.at(minWup)));});
      mdd.addRelaxation(maxWup,[maxWup](auto& out,const auto& l,const auto& r) { out.set(maxWup,std::max(l.at(maxWup), r.at(maxWup)));});
      mdd.addRelaxation(len, [len](auto& out,const auto& l,const auto& r)  { out.set(len,std::max(l.at(len),r.at(len)));});

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
      int nbVars = vars.size();
      std::vector<int> Lproxy(nbVars, 0);
      std::vector<int> Uproxy(nbVars, 0);
      Lproxy[nbVars-1] = 0;
      Uproxy[nbVars-1] = 0;
      for (int i=nbVars-2; i>=0; i--) {
	Lproxy[i] = Lproxy[i+1] + array[i+1]*vars[i+1]->min();
	Uproxy[i] = Uproxy[i+1] + array[i+1]*vars[i+1]->max();	
      }

      auto& d = mdd.makeConstraintDescriptor(vars,"sumMDD");

      // Define the states
      const int minW = mdd.addState(d, 0, INT_MAX);
      const int maxW = mdd.addState(d, 0, INT_MAX);
      const int minWup = mdd.addStateUp(d, 0, INT_MAX);
      const int maxWup = mdd.addStateUp(d, 0, INT_MAX);

      // State 'len' is needed to capture the index i, to express array[i]*val when vars[i]=val.
      const int len  = mdd.addState(d, 0, vars.size());

      mdd.addArc(d,[=] (const auto& p, const auto& c, var<int>::Ptr var, int val,bool upPass) -> bool {
	  if (upPass==true) {
	    return ((p.at(minW) + val*array[p.at(len)] + c.at(minWup) <= z->max()) &&
		    (p.at(maxW) + val*array[p.at(len)] + c.at(maxWup) >= z->min()));
	  } else {
	    return ((p.at(minW) + val*array[p.at(len)] + Lproxy[p.at(len)] <= z->max()) && 
		    (p.at(maxW) + val*array[p.at(len)] + Uproxy[p.at(len)] >= z->min()));
	  }
	});

      
      mdd.addTransition(minW,[minW,array,len] (auto& out,const auto& p,auto var, int val) {
	  out.set(minW, p.at(minW) + array[p.at(len)]*val);});
      mdd.addTransition(maxW,[maxW,array,len] (auto& out,const auto& p,auto var, int val) {
	  out.set(maxW, p.at(maxW) + array[p.at(len)]*val);});

      mdd.addTransition(minWup,[minWup,array,len] (auto& out,const auto& in,auto var, int val) {
	  if (in.at(len) >= 1) {
	    out.set(minWup, in.at(minWup) + array[in.at(len)-1]*val);
	  }
	});
      mdd.addTransition(maxWup,[maxWup,array,len] (auto& out,const auto& in,auto var, int val) {
	  if (in.at(len) >= 1) {
	    out.set(maxWup, in.at(maxWup) + array[in.at(len)-1]*val);
	  }
	});

      
      mdd.addTransition(len, [len]            (auto& out,const auto& p,auto var, int val) {
	  out.set(len,  p.at(len) + 1);});      

      mdd.addRelaxation(minW,[minW](auto& out,const auto& l,const auto& r) { out.set(minW,std::min(l.at(minW), r.at(minW)));});
      mdd.addRelaxation(maxW,[maxW](auto& out,const auto& l,const auto& r) { out.set(maxW,std::max(l.at(maxW), r.at(maxW)));});
      mdd.addRelaxation(minWup,[minWup](auto& out,const auto& l,const auto& r) { out.set(minWup,std::min(l.at(minWup), r.at(minWup)));});
      mdd.addRelaxation(maxWup,[maxWup](auto& out,const auto& l,const auto& r) { out.set(maxWup,std::max(l.at(maxWup), r.at(maxWup)));});
      mdd.addRelaxation(len, [len](auto& out,const auto& l,const auto& r)  { out.set(len,std::max(l.at(len),r.at(len)));});

      mdd.addSimilarity(minW,[minW](auto l,auto r) -> double { return abs(l.at(minW) - r.at(minW)); });
      mdd.addSimilarity(maxW,[maxW](auto l,auto r) -> double { return abs(l.at(maxW) - r.at(maxW)); });
      mdd.addSimilarity(minWup,[minWup](auto l,auto r) -> double { return abs(l.at(minWup) - r.at(minWup)); });
      mdd.addSimilarity(maxWup,[maxWup](auto l,auto r) -> double { return abs(l.at(maxWup) - r.at(maxWup)); });
      mdd.addSimilarity(len ,[] (auto l,auto r) -> double { return 0; }); 
  }  

  void sumMDD(MDDSpec& mdd, const Factory::Veci& vars, const std::vector<std::vector<int>>& matrix, var<int>::Ptr z) {
      // Enforce MDD bounds consistency on
      //   sum(i, matrix[i][vars[i]]) == z 
      mdd.append(vars);

      // Create lower and upper bounds as proxy for bottom-up values.
      // At layer i, the proxy sums the minimum (resp. maximum) value
      // from layers i+1 through n.      
      int nbVars = vars.size();
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
      
      auto& d = mdd.makeConstraintDescriptor(vars,"sumMDD");

      // Define the states
      const int minW = mdd.addState(d, 0, INT_MAX);
      const int maxW = mdd.addState(d, 0, INT_MAX);
      const int minWup = mdd.addStateUp(d, 0, INT_MAX);
      const int maxWup = mdd.addStateUp(d, 0, INT_MAX);

      // State 'len' is needed to capture the index i, to express matrix[i][vars[i]]
      const int len  = mdd.addState(d, 0, vars.size());

      mdd.addArc(d,[=] (const auto& p, const auto& c, var<int>::Ptr var, int val,bool upPass) -> bool {
	  if (upPass==true) {
	    return ((p.at(minW) + matrix[p.at(len)][val] + c.at(minWup) <= z->max()) &&
		    (p.at(maxW) + matrix[p.at(len)][val] + c.at(maxWup) >= z->min()));
	  } else {
	    return ((p.at(minW) + matrix[p.at(len)][val] + Lproxy[p.at(len)] <= z->max()) && 
		    (p.at(maxW) + matrix[p.at(len)][val] + Uproxy[p.at(len)] >= z->min()));
	  }
	});
      
      mdd.addTransition(minW,[minW,matrix,len] (auto& out,const auto& p,auto var, int val) {
	  out.set(minW, p.at(minW) + matrix[p.at(len)][val]);});
      mdd.addTransition(maxW,[maxW,matrix,len] (auto& out,const auto& p,auto var, int val) {
	  out.set(maxW, p.at(maxW) + matrix[p.at(len)][val]);});

      mdd.addTransition(minWup,[minWup,matrix,len] (auto& out,const auto& in,auto var, int val) {
	  if (in.at(len) >= 1) {
	    out.set(minWup, in.at(minWup) + matrix[in.at(len)-1][val]);
	  }
	});
      mdd.addTransition(maxWup,[maxWup,matrix,len] (auto& out,const auto& in,auto var, int val) {
	  if (in.at(len) >= 1) {
	    out.set(maxWup, in.at(maxWup) + matrix[in.at(len)-1][val]);
	  }
	});
      
      mdd.addTransition(len, [len]            (auto& out,const auto& p,auto var, int val) {
	  out.set(len,  p.at(len) + 1);});      

      mdd.addRelaxation(minW,[minW](auto& out,const auto& l,const auto& r) { out.set(minW,std::min(l.at(minW), r.at(minW)));});
      mdd.addRelaxation(maxW,[maxW](auto& out,const auto& l,const auto& r) { out.set(maxW,std::max(l.at(maxW), r.at(maxW)));});
      mdd.addRelaxation(minWup,[minWup](auto& out,const auto& l,const auto& r) { out.set(minWup,std::min(l.at(minWup), r.at(minWup)));});
      mdd.addRelaxation(maxWup,[maxWup](auto& out,const auto& l,const auto& r) { out.set(maxWup,std::max(l.at(maxWup), r.at(maxWup)));});
      mdd.addRelaxation(len, [len](auto& out,const auto& l,const auto& r)  { out.set(len,std::max(l.at(len),r.at(len)));});

      mdd.onFixpoint([z,minW,maxW](const auto& sink) {
                        z->updateBounds(sink.at(minW),sink.at(maxW));
                     });

      mdd.addSimilarity(minW,[minW](auto l,auto r) -> double { return abs(l.at(minW) - r.at(minW)); });
      mdd.addSimilarity(maxW,[maxW](auto l,auto r) -> double { return abs(l.at(maxW) - r.at(maxW)); });
      mdd.addSimilarity(minWup,[minWup](auto l,auto r) -> double { return abs(l.at(minWup) - r.at(minWup)); });
      mdd.addSimilarity(maxWup,[maxWup](auto l,auto r) -> double { return abs(l.at(maxWup) - r.at(maxWup)); });
      mdd.addSimilarity(len ,[] (auto l,auto r) -> double { return 0; }); 
  }  
}

