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
      spec.arcExist(desc,[=] (const auto& p,const auto& c,const auto& x,int v,bool) -> bool {
                          bool inS = values.member(v);
                          int minv = p.at(pmax) - p.at(pmin) + inS;
                          return (p.at(p0) < 0 &&  minv >= lb && p.at(pminL) + inS               <= ub)
                             ||  (p.at(p0) >= 0 && minv >= lb && p.at(pminL) - p.at(pmaxF) + inS <= ub);
                       });
      
      spec.transitionDown(toDict(ps[minFIdx],
                                 ps[minLIdx]-1,
                                 [](int i) {
                                    return tDesc({i+1},[i](auto& out,const auto& p,const auto& x,const auto& val,bool up) {
                                                          out.set(i,p.at(i+1));
                                                       });
                                 }));      
      spec.transitionDown(toDict(ps[maxFIdx],
                                 ps[maxLIdx]-1,
                                 [](int i) {
                                    return tDesc({i+1},[i](auto& out,const auto& p,const auto& x,const auto& val,bool up) {
                                                          out.set(i,p.at(i+1));
                                                       });                                            
                                 }));
      
      spec.transitionDown(ps[minLIdx],{pminL},
                          [values,minL=ps[minLIdx]](auto& out,const auto& p,const auto& x,const auto& val,bool up) {
                             bool allMembers = true;
                             for(int v : val) {
                                allMembers &= values.member(v);
                                if (!allMembers) break;
                             }
                             out.set(minL,p.at(minL)+allMembers);
                          });
      spec.transitionDown(ps[maxLIdx],{ps[maxLIdx]},
                          [values,maxL=ps[maxLIdx]](auto& out,const auto& p,const auto& x,const auto& val,bool up) {
                             bool oneMember = false;
                             for(int v : val) {
                                oneMember = values.member(v);
                                if (oneMember) break;
                             }
                             out.set(maxL,p.at(maxL)+oneMember);
                          });
      
      for(int i = minFIdx; i <= minLIdx; i++)
         spec.addRelaxation(ps[i],[p=ps[i]](auto& out,const auto& l,const auto& r) {
                                     out.set(p,std::min(l.at(p),r.at(p)));
                                  });
      for(int i = maxFIdx; i <= maxLIdx; i++)
         spec.addRelaxation(ps[i],[p=ps[i]](auto& out,const auto& l,const auto& r) {
                                     out.set(p,std::max(l.at(p),r.at(p)));
                                  });
      
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
                                 [](int i) { return tDesc({i+1},
                                                          [i](auto& out,const auto& p,const auto& x,const auto& val,bool up) {
                                                             out.set(i,p.at(i+1));
                                                          });
                                 }));
      spec.transitionDown(toDict(maxF,maxL-1,
                                 [](int i) { return tDesc({i+1},
                                                          [i](auto& out,const auto& p,const auto& x,const auto& val,bool up) {
                                                             out.set(i,p.at(i+1));
                                                          });
                                 }));

      spec.transitionDown(minL,{minL},[values,minL](auto& out,const auto& p,const auto& x,const auto& val,bool up) {
                                 bool allMembers = true;
                                 for(int v : val) {
                                    allMembers &= values.member(v);
                                    if (!allMembers) break;
                                 }
                                 out.set(minL,p.at(minL)+allMembers);
                              });
      spec.transitionDown(maxL,{maxL},[values,maxL](auto& out,const auto& p,const auto& x,const auto& val,bool up) {
                                 bool oneMember = false;
                                 for(int v : val) {
                                    oneMember = values.member(v);
                                    if (oneMember) break;
                                 }
                                 out.set(maxL,p.at(maxL)+oneMember);
                              });
      spec.transitionDown(pnb,{pnb},[pnb](auto& out,const auto& p,const auto& x,const auto& val,bool up) {
                                out.setInt(pnb,p[pnb]+1);
                             });

      spec.arcExist(desc,[=] (const auto& p,const auto& c,const auto& x,int v,bool) -> bool {
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
         spec.addRelaxation(ps[i],[p=ps[i]](auto& out,const auto& l,const auto& r) {
                                     out.set(p,std::min(l.at(p),r.at(p)));
                                  });
      for(int i = maxFIdx; i <= maxLIdx; i++)
         spec.addRelaxation(ps[i],[p=ps[i]](auto& out,const auto& l,const auto& r) {
                                     out.set(p,std::max(l.at(p),r.at(p)));
                                  });
      spec.addRelaxation(pnb,[pnb](auto& out,const auto& l,const auto& r) {
                                out.set(pnb,std::min(l.at(pnb),r.at(pnb)));
                             });
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
      int ExactIdx = NIdx+1; // is Ymin = Ymax?
      
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
      ps[ExactIdx] = spec.addState(desc, 1, 1,MinFun);

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
      const int Exact = ps[ExactIdx];

      /*
      spec.transitionDown(Amin,[](auto& out,const auto& p,const auto& x,const auto&val, bool up) {
                                  MDDWindow ow = out.getWin(Amin);
                                  //ow = p.getWin(Amin) >> 1;
                                  ow.setWithShift(p.getWin(Amin),1);
                                  ow.setFirst(p.at(Ymin));
                               });
      */
      // down transitions
      spec.transitionDown(toDict(AminF+1,AminL,
                                 [](int i) { return tDesc({i-1},
                                                          [i](auto& out,const auto& p,const auto& x,const auto& val,bool up) {
                                                             out.set(i,p.at(i-1));
                                                          });
                                 }));
      
      spec.transitionDown(toDict(AmaxF+1,AmaxL,
                                 [](int i) { return tDesc({i-1},
                                                          [i](auto& out,const auto& p,const auto& x,const auto& val,bool up) {
                                                             out.set(i,p.at(i-1));
                                                          });
                                 }));

      spec.transitionDown(AminF,{Ymin},[AminF,Ymin](auto& out,const auto& p,const auto& x,const auto& val,bool up) {
                                   out.set(AminF,p.at(Ymin));
                                });
      spec.transitionDown(AmaxF,{Ymax},[AmaxF,Ymax](auto& out,const auto& p,const auto& x,const auto& val,bool up) {
                                   out.set(AmaxF,p.at(Ymax));
                                });

      spec.transitionDown(Ymin,{Ymin},[values,Ymin](auto& out,const auto& p,const auto& x,const auto& val,bool up) {
          bool hasMemberOutS = val.memberOutside(values);
	  int minVal = p.at(Ymin) + !hasMemberOutS;
	  if (up) 
             minVal = std::max(minVal, out.at(Ymin));	  
	  out.set(Ymin,minVal);
	});

      spec.transitionDown(Ymax,{Ymax},[values,Ymax](auto& out,const auto& p,const auto& x,const auto& val,bool up) {
          bool hasMemberInS = val.memberInside(values);
	  int maxVal = p.at(Ymax) + hasMemberInS;
	  if (up)
	    maxVal = std::min(maxVal, out.at(Ymax));
	  out.set(Ymax,maxVal);
	});

      spec.transitionDown(N,{N},[N](auto& out,const auto& p,const auto& x,const auto& val,bool up) { out.set(N,p.at(N)+1); });
      spec.transitionDown(Exact,{Exact},[Exact,values](auto& out,const auto& p,const auto& x,const auto& val,bool up) {
	  out.set(Exact, (p.at(Exact)==1) && (val.memberOutside(values) != val.memberInside(values)));
      });

      // up transitions
      spec.transitionUp(toDict(DminF+1,DminL,
                               [](int i) { return tDesc({i-1},
                                                        [i](auto& out,const auto& c,const auto& x,const auto& val,bool up) {
                                                           out.set(i,c.at(i-1));
                                                        });
                               }));
      spec.transitionUp(toDict(DmaxF+1,DmaxL,
                               [](int i) { return tDesc({i-1},
                                                        [i](auto& out,const auto& c,const auto& x,const auto& val,bool up) {
                                                           out.set(i,c.at(i-1));
                                                        });
                               }));
      spec.transitionUp(DminF,{Ymin},[DminF,Ymin](auto& out,const auto& c,const auto& x,const auto& val,bool up) {
                                        out.set(DminF,c.at(Ymin));
                                     });
      spec.transitionUp(DmaxF,{Ymax},[DmaxF,Ymax](auto& out,const auto& c,const auto& x,const auto& val,bool up) {
                                        out.set(DmaxF,c.at(Ymax));
                                     });

      spec.transitionUp(Ymin,{Ymin},[Ymin,values](auto& out,const auto& c,const auto& x,const auto& val,bool up) {
                                bool hasMemberInS = val.memberInside(values);
                                int minVal = std::max(out.at(Ymin), c.at(Ymin) - hasMemberInS);
                                out.set(Ymin,minVal);
                             });

      spec.transitionUp(Ymax,{Ymax},[Ymax,values](auto& out,const auto& c,const auto& x,const auto& val,bool up) {
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
      spec.arcExist(desc,[=] (const auto& p,const auto& c,const auto& x,int v,bool up) -> bool {
                            bool c0 = true,c1 = true,inS = values.member(v);
                            if (up) { // during the initial post, I do test arc existence and up isn't there yet.
                               c0 = (p.at(Ymin) + inS <= c.at(Ymax));
                               c1 = (p.at(Ymax) + inS >= c.at(Ymin));
                            }
                            return c0 && c1;
                         });

      spec.splitOnLargest([Exact,Ymin,Ymax,AminL,DmaxL,lb,ub,nbVars](const auto& in) {

                             // return (double)std::max(lb+in.getState().at(AminL)-in.getState().at(Ymin),0);
                             return (double)(in.getState().at(Exact));
	  // return -(double)(in.getState().at(Ymax)-in.getState().at(Ymin));
	  // return -(double)(in.getState().at(Ymin));
	  // return -(double)(in.getNumParents());
	  /** bad performance **
	  // return (double)(std::max(lb+in.getState().at(AminL)-in.getState().at(Ymin),0)+
	  // 		  std::max(in.getState().at(DmaxL)-lb-in.getState().at(Ymin),0));
	  **/	  

	});      
   }

}
