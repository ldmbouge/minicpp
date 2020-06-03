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
                             MDDSWin<short> outWin = out.getSW(minWin);
                             outWin.assignSlideBy(p.getSW(minWin),1);
                             outWin.setFirst(p.getSW(minWin).first() + allMembers);
                          });
      spec.transitionDown(maxWin,{maxWin},
                          [values,maxWin](auto& out,const auto& p,const auto& x,const auto& val,bool up) {
                             bool oneMember = val.memberInside(values);
                             MDDSWin<short> outWin = out.getSW(maxWin);
                             outWin.assignSlideBy(p.getSW(maxWin),1);
                             outWin.setFirst(p.getSW(maxWin).first() + oneMember);
                          });      
      
   }

   void seqMDD2(MDDSpec& spec,const Factory::Veci& vars, int len, int lb, int ub, std::set<int> rawValues)
   {
      //const int nb = len*2;
      spec.append(vars);
      ValueSet values(rawValues);
      auto desc = spec.makeConstraintDescriptor(vars,"seqMDD");

      int minWin = spec.addSWState(desc,len,-1,0,MinFun);
      int maxWin = spec.addSWState(desc,len,-1,0,MaxFun);
      int pnb    = spec.addState(desc,0,INT_MAX,MinFun); // init @ 0, largest value is number of variables. 

      spec.transitionDown(minWin,{minWin},[values,minWin](auto& out,const auto& p,const auto& x,const auto& val,bool up) {
                                             bool allMembers = val.allInside(values);
                                             MDDSWin<short> outWin = out.getSW(minWin);
                                             outWin.assignSlideBy(p.getSW(minWin),1);
                                             outWin.setFirst(p.getSW(minWin).first() + allMembers);
                                          });
      spec.transitionDown(maxWin,{maxWin},[values,maxWin](auto& out,const auto& p,const auto& x,const auto& val,bool up) {
                                             bool oneMember = val.memberInside(values);
                                             MDDSWin<short> outWin = out.getSW(maxWin);
                                             outWin.assignSlideBy(p.getSW(maxWin),1);
                                             outWin.setFirst(p.getSW(maxWin).first() + oneMember);
                                          });
      spec.transitionDown(pnb,{pnb},[pnb](auto& out,const auto& p,const auto& x,const auto& val,bool up) {
                                       out.setInt(pnb,p[pnb]+1);
                                    });

      spec.arcExist(desc,[=] (const auto& p,const auto& c,const auto& x,int v,bool) -> bool {
                          bool inS = values.member(v);
                          MDDSWin<short> min = p.getSW(minWin);
                          MDDSWin<short> max = p.getSW(maxWin);
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
      spec.append(vars);
      ValueSet values(rawValues);
      auto desc = spec.makeConstraintDescriptor(vars,"seqMDD");

      const int Ymin = spec.addState(desc, 0, INT_MAX,MinFun);
      const int Ymax = spec.addState(desc, 0, INT_MAX,MaxFun);
      const int AminWin = spec.addSWState(desc,len,-1,0,MinFun);
      const int AmaxWin = spec.addSWState(desc,len,-1,0,MaxFun);
      const int DminWin = spec.addSWState(desc,len,-1,0,MinFun);
      const int DmaxWin = spec.addSWState(desc,len,-1,0,MaxFun);
      const int N       = spec.addState(desc, 0, INT_MAX,MinFun);
      const int Exact   = spec.addState(desc, 1, INT_MAX,MinFun);

      // down transitions
      spec.transitionDown(AminWin,{AminWin,Ymin},[AminWin,Ymin](auto& out,const auto& p,const auto& x,const auto& val,bool up) {
                                                    MDDSWin<short> outWin = out.getSW(AminWin);
                                                    outWin.assignSlideBy(p.getSW(AminWin),1);
                                                    outWin.setFirst(p[Ymin]);
                                                 });
      spec.transitionDown(AmaxWin,{AmaxWin,Ymax},[AmaxWin,Ymax](auto& out,const auto& p,const auto& x,const auto& val,bool up) {
                                                    MDDSWin<short> outWin = out.getSW(AmaxWin);
                                                    outWin.assignSlideBy(p.getSW(AmaxWin),1);
                                                    outWin.setFirst(p[Ymax]);
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
      spec.transitionUp(DminWin,{DminWin,Ymin},[DminWin,Ymin](auto& out,const auto& c,const auto& x,const auto& val,bool up) {
                                                  MDDSWin<short> outWin = out.getSW(DminWin);
                                                  outWin.assignSlideBy(c.getSW(DminWin),1);
                                                  outWin.setFirst(c[Ymin]);
                                               });
      spec.transitionUp(DmaxWin,{DmaxWin,Ymax},[DmaxWin,Ymax](auto& out,const auto& c,const auto& x,const auto& val,bool up) {
                                                  MDDSWin<short> outWin = out.getSW(DmaxWin);
                                                  outWin.assignSlideBy(c.getSW(DmaxWin),1);
                                                  outWin.setFirst(c[Ymax]);
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
                            auto Amin = n.getSW(AminWin);
                            auto Amax = n.getSW(AmaxWin);
                            minVal = std::max(lb + Amin.last(),minVal);  // n.getWin(Amin).getLast()
                            maxVal = std::min(ub + Amax.last(),maxVal);
                         }
                         if (n[N] <= nbVars - len) {
                            auto Dmin = n.getSW(DminWin);
                            auto Dmax = n.getSW(DmaxWin);
                            minVal = std::max(Dmin.last() - ub,minVal);
                            maxVal = std::min(Dmax.last() - lb,maxVal);
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

      spec.splitOnLargest([Exact](const auto& in) {
                             return (double)(in.getState()[Exact]);
                          });

      //      spec.splitOnLargest([Exact,Ymin,Ymax,AminWin,DmaxWin,lb,ub,nbVars](const auto& in) {

                             // return (double)std::max(lb+in.getState()[AminL]-in.getState()[Ymin],0);
      //                     return (double)(in.getState()[Exact]);
	  // return -(double)(in.getState()[Ymax]-in.getState()[Ymin]);
	  // return -(double)(in.getState()[Ymin]);
	  // return -(double)(in.getNumParents());
	  /** bad performance **
	  // return (double)(std::max(lb+in.getState()[AminL]-in.getState()[Ymin],0)+
	  // 		  std::max(in.getState()[DmaxL]-lb-in.getState()[Ymin],0));
	  **/	  
      //});      
      spec.equivalenceClassValue([Ymin,Ymax](const auto& p, const auto& c, var<int>::Ptr var, int val) -> int {
         return c[Ymax] - c[Ymin] < 4;
      });
   }

}
