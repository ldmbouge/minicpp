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

      mdd.addArc(d,[=] (const auto& p,const auto& c,var<int>::Ptr var, const auto& val,bool) -> bool {
         return (p.at(minC) + values.member(val) <= ub) &&
                ((p.at(maxC) + values.member(val) +  p.at(rem) - 1) >= lb);
      });

      mdd.transitionDown(minC,[minC,values] (auto& out,const auto& p,auto x, const auto& val,bool up) {
                                bool allMembers = true;
                                for(int v : val) {
                                   allMembers &= values.member(v);
                                   if (!allMembers) break;
                                }
                                out.set(minC,p.at(minC) + allMembers);
                             });
      mdd.transitionDown(maxC,[maxC,values] (auto& out,const auto& p,auto x, const auto& val,bool up) {
                                bool oneMember = false;
                                for(int v : val) {
                                   oneMember = values.member(v);
                                   if (oneMember) break;
                                }
                                out.set(maxC,p.at(maxC) + oneMember);
                             });
      mdd.transitionDown(rem,[rem] (auto& out,const auto& p,auto x,const auto& val,bool up) { out.set(rem,p.at(rem) - 1);});

      mdd.addRelaxation(minC,[minC](auto& out,const auto& l,const auto& r) { out.set(minC,std::min(l.at(minC), r.at(minC)));});
      mdd.addRelaxation(maxC,[maxC](auto& out,const auto& l,const auto& r) { out.set(maxC,std::max(l.at(maxC), r.at(maxC)));});
      mdd.addRelaxation(rem ,[rem](auto& out,const auto& l,const auto& r)  { out.set(rem,std::max(l.at(rem),r.at(rem)));});

      mdd.addSimilarity(minC,[minC](auto l,auto r) -> double { return abs(l.at(minC) - r.at(minC)); });
      mdd.addSimilarity(maxC,[maxC](auto l,auto r) -> double { return abs(l.at(maxC) - r.at(maxC)); });
      mdd.addSimilarity(rem ,[] (auto l,auto r) -> double { return 0; });
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

      mdd.addArc(d,[=] (const auto& p,const auto& c,var<int>::Ptr var, const auto& val, bool up) -> bool {
	  if (up) {
	    return ((p.at(U) + values.member(val) + c.at(Uup) >= lb) &&
		    (p.at(L) + values.member(val) + c.at(Lup) <= ub));
	  }
      });
      
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

      mdd.addArc(d,[minDom,some,all,len,someu,allu,n](const auto& p,const auto& c,auto var,const auto& val,bool up) -> bool  {
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
      const int nbVars = (int)vars.size();
      
      int minFIdx = 0,minLIdx = len-1;
      int maxFIdx = len,maxLIdx = len*2-1;
      int minFIdxUp = len*2,minLIdxUp = len*3-1;
      int maxFIdxUp = len*3,maxLIdxUp = len*4-1;
      int nb = len*4;
      spec.append(vars);
      ValueSet values(rawValues);
      auto desc = spec.makeConstraintDescriptor(vars,"seqMDD");
      std::vector<int> ps(nb+1);
      for(int i = minFIdx;i <= minLIdx;i++)
	ps[i] = spec.addState(desc,0,nbVars);         // init @ 0, largest value is nbVars. 
      for(int i = maxFIdx;i <= maxLIdx;i++)
         ps[i] = spec.addState(desc,0,nbVars);         // init @ nbVars, largest value is nbVars. 
      for(int i = minFIdxUp;i <= minLIdxUp;i++)
         ps[i] = spec.addState(desc,0,nbVars);       // init @ 0, largest value is nbVars. 
      for(int i = maxFIdxUp;i <= maxLIdxUp;i++)
         ps[i] = spec.addState(desc,0,nbVars);       // init @ nbVars, largest value is nbVars. 
      ps[nb] = spec.addState(desc,0,nbVars);   // init @ 0, largest value is number of variables. 

      const int minF = ps[minFIdx];
      const int minL = ps[minLIdx];
      const int maxF = ps[maxFIdx];
      const int maxL = ps[maxLIdx];
      const int minFup = ps[minFIdxUp];
      const int minLup = ps[minLIdxUp];
      const int maxFup = ps[maxFIdxUp];
      const int maxLup = ps[maxLIdxUp];
      const int pnb  = ps[nb];
	
      // down transitions
      spec.transitionDown(toDict(minF,minL-1,
                                 [](int i) { return [i](auto& out,const auto& p,auto x,const auto& val,bool up) { out.set(i,p.at(i+1));};}));
      spec.transitionDown(toDict(maxF,maxL-1,
                                 [](int i) { return [i](auto& out,const auto& p,auto x,const auto& val,bool up) { out.set(i,p.at(i+1));};}));
      spec.transitionDown(minL,[ps,values,minL,minF,minLup,len,pnb,lb,nbVars,ub](auto& out,const auto& p,auto x,const auto& val,bool up) {

	  bool hasMemberOutS = false;
	  
          for(int v : val)  {
	    if (!values.member(v)) { 
	      hasMemberOutS = true;
	      break;
	    }
	  }

	  int minVal = p.at(minL);
	  if (!hasMemberOutS) { minVal++; };
	  if (p.at(pnb) >= len-1) { minVal = std::max(minVal, lb+p.at(minF)); }

	  if (up && out.at(pnb) <= nbVars-len+1) {
	    minVal = std::max(minVal, out.at(minLup) - ub);
	  }
	  
	  out.set(minL,minVal);
	});

      spec.transitionDown(maxL,[values,maxL,maxF,ub](auto& out,const auto& p,auto x,const auto& val,bool up) {
	  
	  bool hasMemberInS = false;
	  
          for(int v : val)  {
	    if (values.member(v)) {
	      hasMemberInS = true;
	      break;
	    }
	  }

	  int maxVal = p.at(maxL) + hasMemberInS;
	  maxVal = std::min(maxVal, ub+p.at(maxF));

	  // Adding this causes an error: testSequence misses a solution!
	  // Need to print out the MDD during search...
	  // if (up && out.at(pnb) <= nbVars-len+1) {
	  //   maxVal = std::min(maxVal, out.at(maxLup) - lb);
	  // }
	  
	  out.set(maxL,maxVal);
	});
      spec.transitionDown(pnb,[pnb](auto& out,const auto& p,auto x,const auto& val,bool up) {
                                out.set(pnb,p.at(pnb)+1);
                             });

      // up transitions
      spec.transitionUp(toDict(minFup,minLup-1,
                               [](int i) { return [i](auto& out,const auto& c,auto x,const auto& val,bool up) { out.set(i,c.at(i+1));};}));
      spec.transitionUp(toDict(maxFup,maxLup-1,
                               [](int i) { return [i](auto& out,const auto& c,auto x,const auto& val,bool up) { out.set(i,c.at(i+1));};}));
      spec.transitionUp(minLup,[values,minLup,minFup,len,pnb,lb,nbVars](auto& out,const auto& c,
                                                                        auto x,const auto& val,bool up) {
	  bool hasMemberOutS = false;
	  
          for(int v : val)  {
	    if (!values.member(v)) {
	      hasMemberOutS = true;
	      break;
	    }
	  }

	  int minVal = c.at(minLup);
	  if (!hasMemberOutS) { minVal++; }
	  if (c.at(pnb) <= nbVars-len+1) { minVal = std::max(minVal, lb+c.at(minFup)); }

	  out.set(minLup,minVal);			   
	});
      spec.transitionUp(maxLup,[values,maxLup,maxFup,ub](auto& out,const auto& c,auto x,const auto& val,bool up) {

	  bool hasMemberInS = false;
	  
          for(int v : val)  {
	    if (values.member(v)) {
	      hasMemberInS = true;
	      break;
	    }
	  }
	  
	  int maxVal = c.at(maxLup) + hasMemberInS;
	  maxVal = std::min(maxVal, ub+c.at(maxFup));
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

          // std::cout << "inside spec.addArc  at layer = " << p.at(pnb) << std::endl;
	  
	  if (up) {
	    // std::cout << "  --> checking arc existence with p.minL = " << p.at(minL) 
	    // 	      << " p.maxL = " << p.at(maxL) 
	    // 	      << " p.minLup = " << p.at(minLup) 
	    // 	      << " p.maxLup = " << p.at(maxLup)  
	    // 	      << " inS = " << inS << std::endl;

	    c0 = (p.at(minL) + inS <= c.at(maxL));
	    c1 = (p.at(maxL) + inS >= c.at(minL));

	    c2 = (c.at(minLup) + inS <= p.at(maxLup));
	    c3 = (c.at(maxLup) + inS >= p.at(minLup));
	  }

	  
	  // if (p.at(pnb) >= len - 1) {
	  //   c0 = p.at(maxL) + inS - p.at(minF) >= lb;
	  //   c1 = p.at(minL) + inS - p.at(maxF) <= ub;
	  // } else {
	  //   c0 = len - (p.at(pnb)+1) + p.at(maxL) + inS >= lb;
	  //   c1 = p.at(minL) + inS <= ub;
	  // }

	  // if (up) {
	  //   if (c.at(pnb) <= nbVars-len+1) {
	  //     c2 = c.at(maxLup) + inS - c.at(minFup) >= lb;
	  //     c3 = c.at(minLup) + inS - c.at(maxFup) <= ub;
	  //   }
	  //   else {
	  //     c2 = c.at(maxLup) + inS - c.at(minFup) >= lb - (c.at(pnb)-(nbVars-len+1));
	  //     c3 = c.at(minLup) + inS <= ub;
	  //   }
	  // }
	  
	  // if (up) {
          //    // this does not work?
          //    //std::cout << "p.at(maxL) = " << p.at(maxL) << " + inS = " << inS << " ? >= " << c.at(minL) << std::endl;
          //   //std::cout << "p.at(minL) = " << p.at(minL) << " + inS = " << inS << " ? <= " << c.at(maxL) << std::endl;
	  //   c4 =( p.at(maxL) + inS >= c.at(minL) &&
	  // 	  p.at(minL) + inS <= c.at(maxL) );
	  //   c5 =( p.at(maxLup)  >= c.at(minLup) + inS &&
	  // 	  p.at(minLup)  <= c.at(maxLup) + inS );
	  // }

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
      mdd.addArc(d,[=] (const auto& p, const auto& c, var<int>::Ptr var, const auto& val,bool upPass) -> bool {
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

      mdd.addArc(d,[=] (const auto& p, const auto& c, var<int>::Ptr var, const auto& val,bool upPass) -> bool {
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

      mdd.addArc(d,[=] (const auto& p, const auto& c, var<int>::Ptr var, const auto& val,bool upPass) -> bool {
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

      mdd.addSimilarity(minW,[minW](auto l,auto r) -> double { return abs(l.at(minW) - r.at(minW)); });
      mdd.addSimilarity(maxW,[maxW](auto l,auto r) -> double { return abs(l.at(maxW) - r.at(maxW)); });
      mdd.addSimilarity(minWup,[minWup](auto l,auto r) -> double { return abs(l[minWup] - r[minWup]); });
      mdd.addSimilarity(maxWup,[maxWup](auto l,auto r) -> double { return abs(l[maxWup] - r[maxWup]); });
      mdd.addSimilarity(len ,[] (auto l,auto r) -> double { return 0; }); 
  }  
}
