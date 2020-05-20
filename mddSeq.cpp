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

      int minWin = spec.addSWState(desc,len,-1,0,MinFun);
      int maxWin = spec.addSWState(desc,len,-1,0,MaxFun);

      spec.arcExist(desc,[minWin,maxWin,lb,ub,values] (const auto& p,const auto& c,const auto& x,int v,bool) -> bool {
                          bool inS = values.member(v);
                          auto min = p.getSW(minWin);
                          auto max = p.getSW(maxWin);
                          int minv = max.first() - min.last() + inS;
                          return (min.last() < 0 &&  minv >= lb && min.first() + inS              <= ub)
                             ||  (min.last() >= 0 && minv >= lb && min.first() - max.last() + inS <= ub);
                       });
            
      spec.transitionDown(minWin,{minWin},
                          [values,minWin](auto& out,const auto& p,const auto& x,const auto& val,bool up) {
                             bool allMembers = val.allInside(values);//true;
                             MDDSWin<char> outWin = out.getSW(minWin);
                             outWin.assignSlideBy(p.getSW(minWin),1);
                             outWin.setFirst(p.getSW(minWin).first() + allMembers);
                          });
      spec.transitionDown(maxWin,{maxWin},
                          [values,maxWin](auto& out,const auto& p,const auto& x,const auto& val,bool up) {
                             bool oneMember = val.memberInside(values);
                             MDDSWin<char> outWin = out.getSW(maxWin);
                             outWin.assignSlideBy(p.getSW(maxWin),1);
                             outWin.setFirst(p.getSW(maxWin).first() + oneMember);
                          });      
      
   }

   void seqMDD2(MDDSpec& spec,const Factory::Veci& vars, int len, int lb, int ub, std::set<int> rawValues)
   {
      const int nb = len*2;
      spec.append(vars);
      ValueSet values(rawValues);
      auto desc = spec.makeConstraintDescriptor(vars,"seqMDD");

      int minWin = spec.addSWState(desc,len,-1,0,MinFun);
      int maxWin = spec.addSWState(desc,len,-1,0,MaxFun);
      int pnb    = spec.addState(desc,0,INT_MAX,MinFun); // init @ 0, largest value is number of variables. 

      spec.transitionDown(minWin,{minWin},[values,minWin](auto& out,const auto& p,const auto& x,const auto& val,bool up) {
                                             bool allMembers = val.allInside(values);
                                             MDDSWin<char> outWin = out.getSW(minWin);
                                             outWin.assignSlideBy(p.getSW(minWin),1);
                                             outWin.setFirst(p.getSW(minWin).first() + allMembers);
                                          });
      spec.transitionDown(maxWin,{maxWin},[values,maxWin](auto& out,const auto& p,const auto& x,const auto& val,bool up) {
                                             bool oneMember = val.memberInside(values);
                                             MDDSWin<char> outWin = out.getSW(maxWin);
                                             outWin.assignSlideBy(p.getSW(maxWin),1);
                                             outWin.setFirst(p.getSW(maxWin).first() + oneMember);
                                          });
      spec.transitionDown(pnb,{pnb},[pnb](auto& out,const auto& p,const auto& x,const auto& val,bool up) {
                                       out.setInt(pnb,p[pnb]+1);
                                    });

      spec.arcExist(desc,[=] (const auto& p,const auto& c,const auto& x,int v,bool) -> bool {
                          bool inS = values.member(v);
                          MDDSWin<char> min = p.getSW(minWin);
                          MDDSWin<char> max = p.getSW(maxWin);
                          if (p[pnb] >= len - 1) {                             
                             bool c0 = max.first() + inS - min.last() >= lb;
                             bool c1 = min.first() + inS - max.last() <= ub;
                             return c0 && c1;
                          } else {
                             bool c0 = len - (p[pnb]+1) + max.first() + inS >= lb;
                             bool c1 =                    min.first() + inS <= ub;
                             return c0 && c1;
                          }
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

      ps[YminIdx] = spec.addState(desc, 0, INT_MAX,MinFun);
      ps[YmaxIdx] = spec.addState(desc, 0, INT_MAX,MaxFun);     
      for(int i = AminFIdx;i <= AminLIdx;i++)	ps[i] = spec.addState(desc, 0, INT_MAX,MinFun);
      for(int i = AmaxFIdx;i <= AmaxLIdx;i++)	ps[i] = spec.addState(desc, 0, INT_MAX,MaxFun);
      for(int i = DminFIdx;i <= DminLIdx;i++)	ps[i] = spec.addState(desc, 0, INT_MAX,MinFun);
      for(int i = DmaxFIdx;i <= DmaxLIdx;i++)	ps[i] = spec.addState(desc, 0, INT_MAX,MaxFun);
      ps[NIdx] = spec.addState(desc, 0, INT_MAX,MinFun);
      ps[ExactIdx] = spec.addState(desc, 1, INT_MAX,MinFun);

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
                                  ow.setFirst(p[Ymin]);
                               });
      */
      // down transitions
      spec.transitionDown(toDict(AminF+1,AminL,
                                 [](int i) { return tDesc({i-1},
                                                          [i](auto& out,const auto& p,const auto& x,const auto& val,bool up) {
                                                             out.setInt(i,p[i-1]);
                                                          });
                                 }));
      
      spec.transitionDown(toDict(AmaxF+1,AmaxL,
                                 [](int i) { return tDesc({i-1},
                                                          [i](auto& out,const auto& p,const auto& x,const auto& val,bool up) {
                                                             out.setInt(i,p[i-1]);
                                                          });
                                 }));

      spec.transitionDown(AminF,{Ymin},[AminF,Ymin](auto& out,const auto& p,const auto& x,const auto& val,bool up) {
                                   out.setInt(AminF,p[Ymin]);
                                });
      spec.transitionDown(AmaxF,{Ymax},[AmaxF,Ymax](auto& out,const auto& p,const auto& x,const auto& val,bool up) {
                                   out.setInt(AmaxF,p[Ymax]);
                                });

      spec.transitionDown(Ymin,{Ymin},[values,Ymin](auto& out,const auto& p,const auto& x,const auto& val,bool up) {
          bool hasMemberOutS = val.memberOutside(values);
	  int minVal = p[Ymin] + !hasMemberOutS;
	  if (up) 
             minVal = std::max(minVal, out[Ymin]);	  
	  out.setInt(Ymin,minVal);
	});

      spec.transitionDown(Ymax,{Ymax},[values,Ymax](auto& out,const auto& p,const auto& x,const auto& val,bool up) {
          bool hasMemberInS = val.memberInside(values);
	  int maxVal = p[Ymax] + hasMemberInS;
	  if (up)
	    maxVal = std::min(maxVal, out[Ymax]);
	  out.setInt(Ymax,maxVal);
	});

      spec.transitionDown(N,{N},[N](auto& out,const auto& p,const auto& x,const auto& val,bool up) { out.setInt(N,p[N]+1); });
      spec.transitionDown(Exact,{Exact},[Exact,values](auto& out,const auto& p,const auto& x,const auto& val,bool up) {
	  out.setInt(Exact, (p[Exact]==1) && (val.memberOutside(values) != val.memberInside(values)));
      });

      // up transitions
      spec.transitionUp(toDict(DminF+1,DminL,
                               [](int i) { return tDesc({i-1},
                                                        [i](auto& out,const auto& c,const auto& x,const auto& val,bool up) {
                                                           out.setInt(i,c[i-1]);
                                                        });
                               }));
      spec.transitionUp(toDict(DmaxF+1,DmaxL,
                               [](int i) { return tDesc({i-1},
                                                        [i](auto& out,const auto& c,const auto& x,const auto& val,bool up) {
                                                           out.setInt(i,c[i-1]);
                                                        });
                               }));
      spec.transitionUp(DminF,{Ymin},[DminF,Ymin](auto& out,const auto& c,const auto& x,const auto& val,bool up) {
                                        out.setInt(DminF,c[Ymin]);
                                     });
      spec.transitionUp(DmaxF,{Ymax},[DmaxF,Ymax](auto& out,const auto& c,const auto& x,const auto& val,bool up) {
                                        out.setInt(DmaxF,c[Ymax]);
                                     });

      spec.transitionUp(Ymin,{Ymin},[Ymin,values](auto& out,const auto& c,const auto& x,const auto& val,bool up) {
                                bool hasMemberInS = val.memberInside(values);
                                int minVal = std::max(out[Ymin], c[Ymin] - hasMemberInS);
                                out.setInt(Ymin,minVal);
                             });

      spec.transitionUp(Ymax,{Ymax},[Ymax,values](auto& out,const auto& c,const auto& x,const auto& val,bool up) {
                                // std::cout << "entering Ymax Up at layer " << c[N] << " with values " << val;
                                bool hasMemberOutS = val.memberOutside(values);
                                int maxVal = std::min(out[Ymax], c[Ymax] - !hasMemberOutS);
                                out.setInt(Ymax,maxVal);
                             });

      spec.updateNode([=](auto& n) {
                         int minVal = n[Ymin];
                         int maxVal = n[Ymax];
                         if (n[N] >= len) {
                            minVal = std::max(lb + n[AminL],minVal);  // n.getWin(Amin).getLast()
                            maxVal = std::min(ub + n[AmaxL],maxVal);
                         }
                         if (n[N] <= nbVars - len) {
                            minVal = std::max(n[DminL] - ub,minVal);
                            maxVal = std::min(n[DmaxL] - lb,maxVal);
			 }
                         n.setInt(Ymin,minVal);
                         n.setInt(Ymax,maxVal);
                      });

      spec.nodeExist(desc,[=](const auto& p) {
	  return ( (p[Ymin] <= p[Ymax]) &&
		   (p[Ymax] >= 0) &&
		   (p[Ymax] <= p[N]) &&
		   (p[Ymin] >= 0) &&
		   (p[Ymin] <= p[N]) );
	});
      
      // arc definitions
      spec.arcExist(desc,[values,Ymin,Ymax](const auto& p,const auto& c,const auto& x,int v,bool up) -> bool {
                            bool c0 = true,c1 = true,inS = values.member(v);
                            if (up) { // during the initial post, I do test arc existence and up isn't there yet.
                               c0 = (p[Ymin] + inS <= c[Ymax]);
                               c1 = (p[Ymax] + inS >= c[Ymin]);
                            }
                            return c0 && c1;
                         });

      spec.splitOnLargest([Exact,Ymin,Ymax,AminL,DmaxL,lb,ub,nbVars](const auto& in) {

                             // return (double)std::max(lb+in.getState()[AminL]-in.getState()[Ymin],0);
                             return (double)(in.getState()[Exact]);
	  // return -(double)(in.getState()[Ymax]-in.getState()[Ymin]);
	  // return -(double)(in.getState()[Ymin]);
	  // return -(double)(in.getNumParents());
	  /** bad performance **
	  // return (double)(std::max(lb+in.getState()[AminL]-in.getState()[Ymin],0)+
	  // 		  std::max(in.getState()[DmaxL]-lb-in.getState()[Ymin],0));
	  **/	  

	});      
   }

}
