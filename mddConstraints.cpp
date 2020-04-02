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
      auto d = mdd.makeConstraintDescriptor(x,"amongMDD");
      const int minC = mdd.addState(d,0,x.size());
      const int maxC = mdd.addState(d,0,x.size());
      const int rem  = mdd.addState(d,(int)x.size(),x.size());

      mdd.addArc(d,[=] (const auto& p,const auto& c,var<int>::Ptr var, int val,bool) -> bool {
         return (p.at(minC) + values.member(val) <= ub) &&
                ((p.at(maxC) + values.member(val) +  p.at(rem) - 1) >= lb);
      });

      mdd.addTransition(minC,[minC,values] (auto& out,const auto& p,auto x, int v,bool up) { out.set(minC,p.at(minC) + values.member(v));});
      mdd.addTransition(maxC,[maxC,values] (auto& out,const auto& p,auto x, int v,bool up) { out.set(maxC,p.at(maxC) + values.member(v));});
      mdd.addTransition(rem,[rem] (auto& out,const auto& p,auto x,int v,bool up)           { out.set(rem,p.at(rem) - 1);});

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
      auto d = mdd.makeConstraintDescriptor(vars,"allDiffMdd");
      auto udom = domRange(vars);
      int minDom = udom.first;
      const int n    = (int)vars.size();
      const int all  = mdd.addBSState(d,udom.second - udom.first + 1,0);
      const int some = mdd.addBSState(d,udom.second - udom.first + 1,0);
      const int len  = mdd.addState(d,0,vars.size());
      const int allu = mdd.addBSStateUp(d,udom.second - udom.first + 1,0);
      const int someu = mdd.addBSStateUp(d,udom.second - udom.first + 1,0);
      
      mdd.addTransition(all,[minDom,all](auto& out,const auto& in,auto var,int val,bool up) {
                               out.setBS(all,in.getBS(all)).set(val - minDom);
                            });
      mdd.addTransition(some,[minDom,some](auto& out,const auto& in,auto var,int val,bool up) {
                                out.setBS(some,in.getBS(some)).set(val - minDom);
                            });
      mdd.addTransition(len,[len](auto& out,const auto& in,auto var,int val,bool up) { out.set(len,in[len] + 1);});
      mdd.addTransition(allu,[minDom,allu](auto& out,const auto& in,auto var,int val,bool up) {
                                  out.setBS(allu,in.getBS(allu)).set(val - minDom);
                               });
      mdd.addTransition(someu,[minDom,someu](auto& out,const auto& in,auto var,int val,bool up) {
                                  out.setBS(someu,in.getBS(someu)).set(val - minDom);
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

      mdd.addArc(d,[minDom,some,all,len,someu,allu,n](const auto& p,const auto& c,auto var,int val,bool up) -> bool  {
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
      spec.addArc(desc,[=] (const auto& p,const auto& c,auto x,int v,bool) -> bool {
                          bool inS = values.member(v);
                          int minv = p.at(pmax) - p.at(pmin) + inS;
                          return (p.at(p0) < 0 &&  minv >= lb && p.at(pminL) + inS               <= ub)
                             ||  (p.at(p0) >= 0 && minv >= lb && p.at(pminL) - p.at(pmaxF) + inS <= ub);
                       });
      
      spec.addTransitions(toDict(ps[minFIdx],
                                 ps[minLIdx]-1,
                                 [](int i) { return [i](auto& out,const auto& p,auto x,int v,bool up) { out.set(i,p.at(i+1));};}));
      spec.addTransitions(toDict(ps[maxFIdx],
                                 ps[maxLIdx]-1,
                                 [](int i) { return [i](auto& out,const auto& p,auto x,int v,bool up) { out.set(i,p.at(i+1));};}));
      spec.addTransition(ps[minLIdx],[values,
                                      k=ps[minLIdx]](auto& out,const auto& p,auto x,int v,bool up) { out.set(k,p.at(k)+values.member(v));});
      spec.addTransition(ps[maxLIdx],[values,
                                      k=ps[maxLIdx]](auto& out,const auto& p,auto x,int v,bool up) { out.set(k,p.at(k)+values.member(v));});
      
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

      spec.addTransitions(toDict(minF,minL-1,
                                 [](int i) { return [i](auto& out,const auto& p,auto x,int v,bool up) { out.set(i,p.at(i+1));};}));
      spec.addTransitions(toDict(maxF,maxL-1,
                                 [](int i) { return [i](auto& out,const auto& p,auto x,int v,bool up) { out.set(i,p.at(i+1));};}));
      spec.addTransition(minL,[values,minL](auto& out,const auto& p,auto x,int v,bool up) { out.set(minL,p.at(minL)+values.member(v));});
      spec.addTransition(maxL,[values,maxL](auto& out,const auto& p,auto x,int v,bool up) { out.set(maxL,p.at(maxL)+values.member(v));});
      spec.addTransition(pnb,[pnb](auto& out,const auto& p,auto x,int v,bool up) {
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
      int minFIdx = 0,minLIdx = len-1;
      int maxFIdx = len,maxLIdx = len*2-1;
      int minFIdxUp = len*2,minLIdxUp = len*3-1;
      int maxFIdxUp = len*3,maxLIdxUp = len*4-1;
      int nb = len*4;
      spec.append(vars);
      ValueSet values(rawValues);
      auto desc = spec.makeConstraintDescriptor(vars,"seqMDD");
      std::vector<int> ps(nb+1);
      for(int i = minFIdx;i < minFIdxUp;i++)
         ps[i] = spec.addState(desc,0,len);         // init @ 0, largest value is length of window. 
      for(int i = minFIdxUp;i < nb;i++)
         ps[i] = spec.addStateUp(desc,0,len);       // init @ 0, largest value is length of window. 
      ps[nb] = spec.addState(desc,0,vars.size());   // init @ 0, largest value is number of variables. 

      const int minF = ps[minFIdx];
      const int minL = ps[minLIdx];
      const int maxF = ps[maxFIdx];
      const int maxL = ps[maxLIdx];
      const int minFup = ps[minFIdxUp];
      const int minLup = ps[minLIdxUp];
      const int maxFup = ps[maxFIdxUp];
      const int maxLup = ps[maxLIdxUp];
      const int pnb  = ps[nb];

      const int nbVars = (int)vars.size();
	
      // down transitions
      spec.addTransitions(toDict(minF,minL-1,
                                 [](int i) { return [i](auto& out,const auto& p,auto x,int v,bool up) { out.set(i,p.at(i+1));};}));
      spec.addTransitions(toDict(maxF,maxL-1,
                                 [](int i) { return [i](auto& out,const auto& p,auto x,int v,bool up) { out.set(i,p.at(i+1));};}));
      spec.addTransition(minL,[ps,values,minL,minF,maxLup,minLup,len,pnb,lb](auto& out,const auto& p,auto x,int v,bool up) {
	  int minVal = p.at(minL)+values.member(v);

	  if (p.at(pnb) >= len-1) {
	    if (p.at(minL)+values.member(v)-p.at(minF) < lb) { minVal = std::max(minVal, p.at(minF)+lb); }
	  }
	  else {
            // The number of possible ones to complete the window can be computed from the 'up' information (if available).
	    // If the up information is not available, we'll take the remaining window of variables.
	    int onesToGo = 0; //len-p.at(pnb);
	    if (!up) { onesToGo = len-p.at(pnb); }
	    else     {
	      onesToGo = p.at(maxLup-1) - p.at(minLup - (len-p.at(pnb))-1);

	      // std::cout << "[";
	      // for (int i=0; i<ps.size(); i++) {
	      // 	std::cout << p.at(ps[i]) << " ";
	      // }
	      // std::cout << "]" << std::endl;
	      // std::cout << "p.at(pnb) = " << p.at(pnb) << std::endl;
	      // std::cout << "p.at(maxLup-1) = " << p.at(maxLup-1) << std::endl;
	      // std::cout << "p.at(maxLup - (len-p.at(pnb)) = " << p.at(maxLup - (len-p.at(pnb))) << std::endl;
	      // std::cout << "onesToGo = " << onesToGo << std::endl;
	    }

	    if (p.at(minL)+values.member(v) + onesToGo < lb) {
	      minVal = std::max(minVal, lb - onesToGo );
	    }
	  }
	  out.set(minL,minVal);
	});
      spec.addTransition(maxL,[values,maxL,maxF,ub](auto& out,const auto& p,auto x,int v,bool up) {
	  int maxVal = p.at(maxL)+values.member(v);
	  if (p.at(maxL)+values.member(v)-p.at(maxF) > ub) { maxVal = std::min(maxVal, p.at(maxF)+ub); }
	  out.set(maxL,maxVal);
	});
      spec.addTransition(pnb,[pnb](auto& out,const auto& p,auto x,int v,bool up) {
                                out.set(pnb,p.at(pnb)+1);
                             });

      // up transitions
      spec.addTransitions(toDict(minFup,minLup-1,
                                 [](int i) { return [i](auto& out,const auto& c,auto x,int v,bool up) { out.set(i,c.at(i+1));};}));
      spec.addTransitions(toDict(maxFup,maxLup-1,
                                 [](int i) { return [i](auto& out,const auto& c,auto x,int v,bool up) { out.set(i,c.at(i+1));};}));
      spec.addTransition(minLup,[values,minLup,minFup,maxL,minL,len,pnb,lb,nbVars](auto& out,const auto& c,auto x,int v,bool up) {
	  int minVal = c.at(minLup)+values.member(v);

	  if (c.at(pnb) <= nbVars-len+1) {
	    if (c.at(minLup)+values.member(v)-c.at(minFup) < lb) { minVal = std::max(minVal, c.at(minFup)+lb); }
	  }
	  else {
	    int onesToGo = 0;
	    if (!up) { onesToGo = c.at(pnb)-(nbVars-len+1); }
	    //else     { onesToGo = c.at(maxL-1) - c.at(minL - (c.at(pnb)-(nbVars-len+1))); }
	    // problem: the c state does not capture the entire window in the boundary case (at terminal).
	    // resolve by changing to "out" state.
	    else     { onesToGo = out.at(maxL) - out.at(minL - (c.at(pnb)-(nbVars-len+1))); } 
	    if (c.at(minLup)+values.member(v) + onesToGo < lb) {
	      minVal = std::max(minVal, lb - onesToGo);
	    }
	  }
	  out.set(minLup,minVal);
	});
      spec.addTransition(maxLup,[values,maxLup,maxFup,ub](auto& out,const auto& c,auto x,int v,bool up) {
	  int maxVal = c.at(maxLup)+values.member(v);
	  if (c.at(maxLup)+values.member(v)-c.at(maxFup) > ub) { maxVal = std::min(maxVal, c.at(maxFup)+ub); }
	  out.set(maxLup,maxVal);
	});

      // arc definitions
      spec.addArc(desc,[=] (const auto& p,const auto& c,auto x,int v,bool up) -> bool {
                          bool inS = values.member(v);

			  bool c0 = true;
			  bool c1 = true;
			  bool c2 = true;
			  bool c3 = true;
			  bool c4 = true;
			  bool c5 = true;
			  
			  if (p.at(pnb) >= len - 1) {
			    c0 = p.at(maxL) + inS - p.at(minF) >= lb;
			    c1 = p.at(minL) + inS - p.at(maxF) <= ub;
			  } else {
			    c0 = len - (p.at(pnb)+1) + p.at(maxL) + inS >= lb;
			    c1 = p.at(minL) + inS <= ub;
			  }
			  if (up) {
			    if (c.at(pnb) <= nbVars-len+1) {
			      c2 = c.at(maxLup) + inS - c.at(minFup) >= lb;
			      c3 = c.at(minLup) + inS - c.at(maxFup) <= ub;
			    }
			    else {
			      c2 = c.at(maxLup) + inS - c.at(minFup) >= lb - (c.at(pnb)-(nbVars-len+1));
			      c3 = c.at(minLup) + inS <= ub;
			    }
			  }

			  if (up) {
             // this does not work?
             //std::cout << "p.at(maxL) = " << p.at(maxL) << " + inS = " << inS << " ? >= " << c.at(minL) << std::endl;
            //std::cout << "p.at(minL) = " << p.at(minL) << " + inS = " << inS << " ? <= " << c.at(maxL) << std::endl;
                             c4 =( p.at(maxL) + inS >= c.at(minL) &&
                                   p.at(minL) + inS <= c.at(maxL) );
                             c5 =( p.at(maxLup)  >= c.at(minLup) + inS &&
                                   p.at(minLup)  <= c.at(maxLup) + inS );
			  }

			  return c0 && c1 && c2 && c3 && c4 && c5;
	});      
      
      // relaxations
      for(int i = minFIdx; i <= minLIdx; i++)
         spec.addRelaxation(ps[i],[p=ps[i]](auto& out,const auto& l,const auto& r) { out.set(p,std::min(l.at(p),r.at(p)));});
      for(int i = maxFIdx; i <= maxLIdx; i++)
         spec.addRelaxation(ps[i],[p=ps[i]](auto& out,const auto& l,const auto& r) { out.set(p,std::max(l.at(p),r.at(p)));});
      for(int i = minFIdxUp; i <= minLIdxUp; i++)
         spec.addRelaxation(ps[i],[p=ps[i]](auto& out,const auto& l,const auto& r) { out.set(p,std::min(l.at(p),r.at(p)));});
      for(int i = maxFIdxUp; i <= maxLIdxUp; i++)
         spec.addRelaxation(ps[i],[p=ps[i]](auto& out,const auto& l,const auto& r) { out.set(p,std::max(l.at(p),r.at(p)));});
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
      auto desc = spec.makeConstraintDescriptor(vars,"gccMDD");

      std::vector<int> ps = spec.addStates(desc,minFDom, maxLDom,sz,[] (int i) -> int { return 0; });

      spec.addArc(desc,[=](const auto& p,const auto& c,auto x,int v,bool)->bool{
                          return p.at(ps[v-min]) < values[v];
                       });

      lambdaMap d = toDict(minFDom,maxLDom,ps,[dz,min,minLDom,ps] (int i,int pi) -> lambdaTrans {
              if (i <= minLDom)
                 return [=] (auto& out,const auto& p,auto x, int v,bool up) { out.set(pi,p.at(pi) + ((v - min) == i));};
              return [=] (auto& out,const auto& p,auto x, int v,bool up)    { out.set(pi,p.at(pi) + ((v - min) == (i - dz)));};
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
	
      mdd.addTransition(minW,[minW,array,len] (auto& out,const auto& p,auto var, int val,bool up) {
	  out.set(minW, p.at(minW) + array[p.at(len)]*val);});
      mdd.addTransition(maxW,[maxW,array,len] (auto& out,const auto& p,auto var, int val,bool up) {
	  out.set(maxW, p.at(maxW) + array[p.at(len)]*val);});

      mdd.addTransition(minWup,[minWup,array,len] (auto& out,const auto& in,auto var, int val,bool up) {
	  if (in.at(len) >= 1) {
	    out.set(minWup, in.at(minWup) + array[in.at(len)-1]*val);
	  }
	});
      mdd.addTransition(maxWup,[maxWup,array,len] (auto& out,const auto& in,auto var, int val,bool up) {
	  if (in.at(len) >= 1) {
	    out.set(maxWup, in.at(maxWup) + array[in.at(len)-1]*val);
	  }
	});
      
      mdd.addTransition(len, [len] (auto& out,const auto& p,auto var, int val,bool up) {
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

      
      mdd.addTransition(minW,[minW,array,len] (auto& out,const auto& p,auto var, int val,bool up) {
	  out.set(minW, p.at(minW) + array[p.at(len)]*val);});
      mdd.addTransition(maxW,[maxW,array,len] (auto& out,const auto& p,auto var, int val,bool up) {
	  out.set(maxW, p.at(maxW) + array[p.at(len)]*val);});

      mdd.addTransition(minWup,[minWup,array,len] (auto& out,const auto& in,auto var, int val,bool up) {
	  if (in.at(len) >= 1) {
	    out.set(minWup, in.at(minWup) + array[in.at(len)-1]*val);
	  }
	});
      mdd.addTransition(maxWup,[maxWup,array,len] (auto& out,const auto& in,auto var, int val,bool up) {
	  if (in.at(len) >= 1) {
	    out.set(maxWup, in.at(maxWup) + array[in.at(len)-1]*val);
	  }
	});

      
      mdd.addTransition(len, [len]            (auto& out,const auto& p,auto var, int val,bool up) {
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
      const int minWup = mdd.addStateUp(d, 0, INT_MAX);
      const int maxWup = mdd.addStateUp(d, 0, INT_MAX);

      // State 'len' is needed to capture the index i, to express matrix[i][vars[i]]
      const int len  = mdd.addState(d, 0, vars.size());

      mdd.addArc(d,[=] (const auto& p, const auto& c, var<int>::Ptr var, int val,bool upPass) -> bool {
                      const int mlv = matrix[p[len]][val];
                      if (upPass==true) {
                         return ((p[minW] + mlv + c[minWup] <= z->max()) &&
                                 (p[maxW] + mlv + c[maxWup] >= z->min()));
                      } else {
                         return ((p[minW] + mlv + Lproxy[p[len]] <= z->max()) && 
                                 (p[maxW] + mlv + Uproxy[p[len]] >= z->min()));
                      }
                   });
      
      mdd.addTransition(minW,[minW,matrix,len] (auto& out,const auto& p,auto var, int val,bool up) {
	  out.setInt(minW, p[minW] + matrix[p[len]][val]);});
      mdd.addTransition(maxW,[maxW,matrix,len] (auto& out,const auto& p,auto var, int val,bool up) {
	  out.setInt(maxW, p[maxW] + matrix[p[len]][val]);});

      mdd.addTransition(minWup,[minWup,matrix,len] (auto& out,const auto& in,auto var, int val,bool up) {
	  if (in[len] >= 1) {
             out.setInt(minWup, in[minWup] + matrix[in[len]-1][val]);
	  }
	});
      mdd.addTransition(maxWup,[maxWup,matrix,len] (auto& out,const auto& in,auto var, int val,bool up) {
	  if (in.at(len) >= 1) {
             out.setInt(maxWup, in[maxWup] + matrix[in[len]-1][val]);
	  }
	});
      
      mdd.addTransition(len, [len]            (auto& out,const auto& p,auto var, int val,bool up) {
                                out.setInt(len,  p[len] + 1);});      

      mdd.addRelaxation(minW,[minW](auto& out,const auto& l,const auto& r) { out.setInt(minW,std::min(l[minW], r[minW]));});
      mdd.addRelaxation(maxW,[maxW](auto& out,const auto& l,const auto& r) { out.setInt(maxW,std::max(l[maxW], r[maxW]));});
      mdd.addRelaxation(minWup,[minWup](auto& out,const auto& l,const auto& r) { out.setInt(minWup,std::min(l[minWup], r[minWup]));});
      mdd.addRelaxation(maxWup,[maxWup](auto& out,const auto& l,const auto& r) { out.setInt(maxWup,std::max(l[maxWup], r[maxWup]));});
      mdd.addRelaxation(len, [len](auto& out,const auto& l,const auto& r)  { out.setInt(len,std::max(l[len],r[len]));});

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

