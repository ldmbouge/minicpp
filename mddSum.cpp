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
	
      mdd.transitionDown(minW,{len,minW},[minW,array,len] (auto& out,const auto& p,const auto& var, const auto& val,bool up) {
                                int delta = std::numeric_limits<int>::max();
                                auto coef = array[p[len]];
                                for(int v : val)
                                   delta = std::min(delta,coef*v);
                                out.setInt(minW, p[minW] + delta);
                                //out.setInt(minW, p[minW] + array[p[len]]*val);
                             });
      mdd.transitionDown(maxW,{len,maxW},[maxW,array,len] (auto& out,const auto& p,const auto& var, const auto& val,bool up) {
                                int delta = std::numeric_limits<int>::min();
                                auto coef = array[p[len]];
                                for(int v : val)
                                   delta = std::max(delta,coef*v);
                                out.setInt(maxW, p[maxW] + delta);
                                //out.setInt(maxW, p[maxW] + array[p[len]]*val);
                             });

      mdd.transitionUp(minWup,{len,minWup},[minWup,array,len] (auto& out,const auto& in,const auto& var, const auto& val,bool up) {
                                  if (in[len] >= 1) {
                                     int delta = std::numeric_limits<int>::max();
                                     auto coef = array[in[len]-1];
                                     for(int v : val)
                                        delta = std::min(delta,coef*v);
                                     out.setInt(minWup, in[minWup] + delta);
                                  }
                               });
      mdd.transitionUp(maxWup,{len,maxWup},[maxWup,array,len] (auto& out,const auto& in,const auto& var, const auto& val,bool up) {
                                  if (in[len] >= 1) {
                                     int delta = std::numeric_limits<int>::min();
                                     auto coef = array[in[len]-1];
                                     for(int v : val)
                                        delta = std::max(delta,coef*v);
                                     out.setInt(maxWup, in[maxWup] + delta);
                                  }
                               });
      
      mdd.transitionDown(len,{len},[len] (auto& out,const auto& p,const auto& var, const auto& val,bool up) {
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
      const int minW = mdd.addState(d, 0, INT_MAX,MinFun);
      const int maxW = mdd.addState(d, 0, INT_MAX,MaxFun);
      const int minWup = mdd.addState(d, 0, INT_MAX,MinFun);
      const int maxWup = mdd.addState(d, 0, INT_MAX,MaxFun);
      // State 'len' is needed to capture the index i, to express array[i]*val when vars[i]=val.
      const int len  = mdd.addState(d, 0, vars.size(),MaxFun);

      mdd.arcExist(d,[=] (const auto& p, const auto& c, var<int>::Ptr var, const auto& val,bool upPass) -> bool {
	  if (upPass==true) {
	    return ((p[minW] + val*array[p[len]] + c[minWup] <= z->max()) &&
		    (p[maxW] + val*array[p[len]] + c[maxWup] >= z->min()));
	  } else {
	    return ((p[minW] + val*array[p[len]] + Lproxy[p[len]] <= z->max()) && 
		    (p[maxW] + val*array[p[len]] + Uproxy[p[len]] >= z->min()));
	  }
	});

      
      mdd.transitionDown(minW,{len,minW},[minW,array,len] (auto& out,const auto& p,const auto& var, const auto& val,bool up) {
                                int delta = std::numeric_limits<int>::max();
                                auto coef = array[p[len]];
                                for(int v : val)
                                   delta = std::min(delta,coef * v);
                                out.setInt(minW,p[minW] + delta);
                             });
      mdd.transitionDown(maxW,{len,maxW},[maxW,array,len] (auto& out,const auto& p,const auto& var, const auto& val,bool up) {
                                int delta = std::numeric_limits<int>::min();
                                auto coef = array[p[len]];
                                for(int v : val)
                                   delta = std::max(delta,coef*v);
                                out.setInt(maxW, p[maxW] + delta);
                             });

      mdd.transitionUp(minWup,{len,minWup},[minWup,array,len] (auto& out,const auto& in,const auto& var, const auto& val,bool up) {
                                 if (in[len] >= 1) {
                                    int delta = std::numeric_limits<int>::max();
                                    auto coef = array[in[len]-1];
                                    for(int v : val)
                                       delta = std::min(delta,coef*v);
                                    out.setInt(minWup, in[minWup] + delta);
                                 }
                              });
      mdd.transitionUp(maxWup,{len,maxWup},[maxWup,array,len] (auto& out,const auto& in,const auto& var, const auto& val,bool up) {
                                 if (in[len] >= 1) {
                                    int delta = std::numeric_limits<int>::min();
                                    auto coef = array[in[len]-1];
                                    for(int v : val)
                                       delta = std::max(delta,coef*v);
                                    out.setInt(maxWup, in[maxWup] + delta);
                                 }
                              });

      
      mdd.transitionDown(len,{len},[len](auto& out,const auto& p,const auto& var, const auto& val,bool up) {
                                out.setInt(len,p[len] + 1);
                             });      

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
      const int minW = mdd.addState(d, 0, INT_MAX,MinFun);
      const int maxW = mdd.addState(d, 0, INT_MAX,MaxFun);
      const int minWup = mdd.addState(d, 0, INT_MAX,MinFun);
      const int maxWup = mdd.addState(d, 0, INT_MAX,MaxFun);
      // State 'len' is needed to capture the index i, to express matrix[i][vars[i]]
      const int len  = mdd.addState(d, 0, vars.size(),MaxFun);

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
      
      mdd.transitionDown(minW,{len,minW},[minW,matrix,len] (auto& out,const auto& p,const auto& var, const auto& val,bool up) {
                                int delta = std::numeric_limits<int>::max();
                                const auto& row = matrix[p[len]];
                                for(int v : val)
                                   delta = std::min(delta,row[v]);
                                out.setInt(minW,p[minW] + delta);
                                //out.setInt(minW, p[minW] + matrix[p[len]][val]);
                             });
      mdd.transitionDown(maxW,{len,maxW},[maxW,matrix,len] (auto& out,const auto& p,const auto& var,const auto& val,bool up) {
                                int delta = std::numeric_limits<int>::min();
                                const auto& row = matrix[p[len]];
                                for(int v : val)
                                   delta = std::max(delta,row[v]);
                                out.setInt(maxW,p[maxW] + delta);
                                //out.setInt(maxW, p[maxW] + matrix[p[len]][val]);
                             });

      mdd.transitionUp(minWup,{len,minWup},[minWup,matrix,len] (auto& out,const auto& in,const auto& var,const auto& val,bool up) {
                                  if (in[len] >= 1) {
                                     int delta = std::numeric_limits<int>::max();
                                     const auto& row = matrix[in[len]-1];
                                     for(int v : val)
                                        delta = std::min(delta,row[v]);
                                     out.setInt(minWup, in[minWup] + delta);
                                     //out.setInt(minWup, in[minWup] + matrix[in[len]-1][val]);
                                  }
                               });
      mdd.transitionUp(maxWup,{len,maxWup},[maxWup,matrix,len] (auto& out,const auto& in,const auto& var,const auto& val,bool up) {
                                  if (in.at(len) >= 1) {
                                     int delta = std::numeric_limits<int>::min();
                                     const auto& row = matrix[in[len]-1];
                                     for(int v : val)
                                        delta = std::max(delta,row[v]);
                                     out.setInt(maxWup, in[maxWup] + delta);
                                     //out.setInt(maxWup, in[maxWup] + matrix[in[len]-1][val]);
                                  }
                               });
      
      mdd.transitionDown(len,{len},[len](auto& out,const auto& p,const auto& var, const auto& val,bool up) {
                                out.setInt(len,  p[len] + 1);
                             });      

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
