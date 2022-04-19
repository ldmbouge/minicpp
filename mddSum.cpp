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
      const int minW = mdd.addDownState(d, 0, INT_MAX);
      const int maxW = mdd.addDownState(d, 0, INT_MAX);
      const int minWup = mdd.addUpState(d, 0, INT_MAX);
      const int maxWup = mdd.addUpState(d, 0, INT_MAX);

      // State 'len' is needed to capture the index i, to express array[i]*val when vars[i]=val.
      const int len  = mdd.addDownState(d, 0, vars.size());
      const int lenUp  = mdd.addUpState(d, 0, vars.size());

      // The lower bound needs the bottom-up state information to be effective.
      mdd.arcExist(d,[=] (const auto& pDown, const auto& pCombined, const auto& cUp, const auto& cCombined, var<int>::Ptr var, const auto& val,bool upPass) -> bool {
         if (upPass==true) {
            return ((pDown[minW] + val*array[pDown[len]] + cUp[minWup] <= ub) &&
                    (pDown[maxW] + val*array[pDown[len]] + cUp[maxWup] >= lb));
         } else {
            return ((pDown[minW] + val*array[pDown[len]] + Lproxy[pDown[len]] <= ub) &&
                    (pDown[maxW] + val*array[pDown[len]] + Uproxy[pDown[len]] >= lb));
         }
      });

      mdd.transitionDown(minW,{len,minW},{},[minW,array,len] (auto& out,const auto& pDown,const auto& pCombined,const auto& var, const auto& val,bool up) {
         int delta = std::numeric_limits<int>::max();
         auto coef = array[pDown[len]];
         for(int v : val)
            delta = std::min(delta,coef*v);
         out.setInt(minW, pDown[minW] + delta);
      });
      mdd.transitionDown(maxW,{len,maxW},{},[maxW,array,len] (auto& out,const auto& pDown,const auto& pCombined,const auto& var, const auto& val,bool up) {
         int delta = std::numeric_limits<int>::min();
         auto coef = array[pDown[len]];
         for(int v : val)
            delta = std::max(delta,coef*v);
         out.setInt(maxW, pDown[maxW] + delta);
      });

      mdd.transitionUp(minWup,{lenUp,minWup},{},[nbVars,minWup,array,lenUp] (auto& out,const auto& cUp,const auto& cCombined,const auto& var, const auto& val,bool up) {
         if (cUp[lenUp] < nbVars) {
            int delta = std::numeric_limits<int>::max();
            auto coef = array[nbVars - cUp[lenUp]-1];
            for(int v : val)
               delta = std::min(delta,coef*v);
            out.setInt(minWup, cUp[minWup] + delta);
         }
      });
      mdd.transitionUp(maxWup,{lenUp,maxWup},{},[nbVars,maxWup,array,lenUp] (auto& out,const auto& cUp,const auto& cCombined,const auto& var, const auto& val,bool up) {
         if (cUp[lenUp] < nbVars) {
            int delta = std::numeric_limits<int>::min();
            auto coef = array[nbVars - cUp[lenUp]-1];
            for(int v : val)
               delta = std::max(delta,coef*v);
            out.setInt(maxWup, cUp[maxWup] + delta);
         }
      });

      mdd.transitionDown(len,{len},{},[len] (auto& out,const auto& pDown,const auto& pCombined,const auto& var, const auto& val,bool up) {
         out.setInt(len,  pDown[len] + 1);
      });
      mdd.transitionUp(lenUp,{lenUp},{},[lenUp] (auto& out,const auto& cUp,const auto& cCombined,const auto& var, const auto& val,bool up) {
         out.setInt(lenUp,cUp[lenUp] + 1);
      });

      mdd.addRelaxationDown(minW,[minW](auto& out,const auto& l,const auto& r) { out.setInt(minW,std::min(l[minW], r[minW]));});
      mdd.addRelaxationDown(maxW,[maxW](auto& out,const auto& l,const auto& r) { out.setInt(maxW,std::max(l[maxW], r[maxW]));});
      mdd.addRelaxationUp(minWup,[minWup](auto& out,const auto& l,const auto& r) { out.setInt(minWup,std::min(l[minWup], r[minWup]));});
      mdd.addRelaxationUp(maxWup,[maxWup](auto& out,const auto& l,const auto& r) { out.setInt(maxWup,std::max(l[maxWup], r[maxWup]));});
      mdd.addRelaxationDown(len, [len](auto& out,const auto& l,const auto& r)  { out.setInt(len,std::max(l[len],r[len]));});
      mdd.addRelaxationUp(lenUp, [lenUp](auto& out,const auto& l,const auto& r)  { out.setInt(lenUp,std::max(l[lenUp],r[lenUp]));});
   }

   void sumMDD(MDDSpec& mdd, const Factory::Vecb& vars, var<int>::Ptr z) {
      // Enforce MDD bounds consistency on
      //   sum(i, vars[i]) == z
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
         Lproxy[i] = Lproxy[i+1] + vars[i+1]->min();
         Uproxy[i] = Uproxy[i+1] + vars[i+1]->max();
      }

      auto d = mdd.makeConstraintDescriptor(vars,"sumMDD");

      // Define the states
      const int minW = mdd.addDownState(d, 0, INT_MAX,MinFun);
      const int maxW = mdd.addDownState(d, 0, INT_MAX,MaxFun);
      const int minWup = mdd.addUpState(d, 0, INT_MAX,MinFun);
      const int maxWup = mdd.addUpState(d, 0, INT_MAX,MaxFun);
      // State 'len' is needed to capture the index i, to express val when vars[i]=val.
      const int len  = mdd.addDownState(d, 0, vars.size(),MaxFun);
      const int lenUp  = mdd.addUpState(d, 0, vars.size(),MaxFun);

      mdd.arcExist(d,[=] (const auto& pDown, const auto& pCombined, const auto& cUp, const auto& cCombined, var<int>::Ptr var, const auto& val,bool upPass) -> bool {
         if (upPass==true) {
            return ((pDown[minW] + val + cUp[minWup] <= z->max()) &&
                    (pDown[maxW] + val + cUp[maxWup] >= z->min()));
         } else {
            return ((pDown[minW] + val + Lproxy[pDown[len]] <= z->max()) &&
                    (pDown[maxW] + val + Uproxy[pDown[len]] >= z->min()));
         }
      });
 
      mdd.nodeExist([=](const auto& down, const auto& up, const auto& combined) {
        return (down[minW] + up[minWup] <= z->max()) &&
               (down[maxW] + up[maxWup] >= z->min());
      });

      mdd.transitionDown(minW,{minW},{},[minW] (auto& out,const auto& pDown,const auto& pCombined,const auto& var, const auto& val,bool up) {
         int delta = std::numeric_limits<int>::max();
         for(int v : val)
            delta = std::min(delta,v);
         out.setInt(minW,pDown[minW] + delta);
      });
      mdd.transitionDown(maxW,{maxW},{},[maxW] (auto& out,const auto& pDown,const auto& pCombined,const auto& var, const auto& val,bool up) {
         int delta = std::numeric_limits<int>::min();
         for(int v : val)
            delta = std::max(delta,v);
         out.setInt(maxW, pDown[maxW] + delta);
      });

      mdd.transitionUp(minWup,{lenUp,minWup},{},[nbVars,minWup,lenUp] (auto& out,const auto& cUp,const auto& cCombined,const auto& var, const auto& val,bool up) {
         if (cUp[lenUp] < nbVars) {
            int delta = std::numeric_limits<int>::max();
            for(int v : val)
               delta = std::min(delta,v);
            out.setInt(minWup, cUp[minWup] + delta);
         }
      });
      mdd.transitionUp(maxWup,{lenUp,maxWup},{},[nbVars,maxWup,lenUp] (auto& out,const auto& cUp,const auto& cCombined,const auto& var, const auto& val,bool up) {
         if (cUp[lenUp] < nbVars) {
            int delta = std::numeric_limits<int>::min();
            for(int v : val)
               delta = std::max(delta,v);
            out.setInt(maxWup, cUp[maxWup] + delta);
         }
      });

      mdd.transitionDown(len,{len},{},[len](auto& out,const auto& pDown,const auto& pCombined,const auto& var, const auto& val,bool up) {
         out.setInt(len,pDown[len] + 1);
      });
      mdd.transitionUp(lenUp,{lenUp},{},[lenUp](auto& out,const auto& cUp,const auto& cCombined,const auto& var, const auto& val,bool up) {
         out.setInt(lenUp,cUp[lenUp] + 1);
      });
      mdd.onFixpoint([z,minW,maxW](const auto& sinkDown,const auto& sinkUp,const auto& sinkCombined) {
         z->updateBounds(sinkDown.at(minW),sinkDown.at(maxW));
      });
   }
   void sumMDD(MDDSpec& mdd, const Factory::Veci& vars, var<int>::Ptr z) {
      // Enforce MDD bounds consistency on
      //   sum(i, vars[i]) == z
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
         Lproxy[i] = Lproxy[i+1] + vars[i+1]->min();
         Uproxy[i] = Uproxy[i+1] + vars[i+1]->max();
      }

      auto d = mdd.makeConstraintDescriptor(vars,"sumMDD");

      // Define the states
      const int minW = mdd.addDownState(d, 0, INT_MAX,MinFun);
      const int maxW = mdd.addDownState(d, 0, INT_MAX,MaxFun);
      const int minWup = mdd.addUpState(d, 0, INT_MAX,MinFun);
      const int maxWup = mdd.addUpState(d, 0, INT_MAX,MaxFun);
      // State 'len' is needed to capture the index i, to express val when vars[i]=val.
      const int len  = mdd.addDownState(d, 0, vars.size(),MaxFun);
      const int lenUp  = mdd.addUpState(d, 0, vars.size(),MaxFun);

      mdd.arcExist(d,[=] (const auto& pDown, const auto& pCombined, const auto& cUp, const auto& cCombined, var<int>::Ptr var, const auto& val,bool upPass) -> bool {
         if (upPass==true) {
            return ((pDown[minW] + val + cUp[minWup] <= z->max()) &&
                    (pDown[maxW] + val + cUp[maxWup] >= z->min()));
         } else {
            return ((pDown[minW] + val + Lproxy[pDown[len]] <= z->max()) &&
                    (pDown[maxW] + val + Uproxy[pDown[len]] >= z->min()));
         }
      });
 
      mdd.nodeExist([=](const auto& down, const auto& up, const auto& combined) {
        return (down[minW] + up[minWup] <= z->max()) &&
               (down[maxW] + up[maxWup] >= z->min());
      });

      mdd.transitionDown(minW,{minW},{},[minW] (auto& out,const auto& pDown,const auto& pCombined,const auto& var, const auto& val,bool up) {
         int delta = std::numeric_limits<int>::max();
         for(int v : val)
            delta = std::min(delta,v);
         out.setInt(minW,pDown[minW] + delta);
      });
      mdd.transitionDown(maxW,{maxW},{},[maxW] (auto& out,const auto& pDown,const auto& pCombined,const auto& var, const auto& val,bool up) {
         int delta = std::numeric_limits<int>::min();
         for(int v : val)
            delta = std::max(delta,v);
         out.setInt(maxW, pDown[maxW] + delta);
      });

      mdd.transitionUp(minWup,{lenUp,minWup},{},[nbVars,minWup,lenUp] (auto& out,const auto& cUp,const auto& cCombined,const auto& var, const auto& val,bool up) {
         if (cUp[lenUp] < nbVars) {
            int delta = std::numeric_limits<int>::max();
            for(int v : val)
               delta = std::min(delta,v);
            out.setInt(minWup, cUp[minWup] + delta);
         }
      });
      mdd.transitionUp(maxWup,{lenUp,maxWup},{},[nbVars,maxWup,lenUp] (auto& out,const auto& cUp,const auto& cCombined,const auto& var, const auto& val,bool up) {
         if (cUp[lenUp] < nbVars) {
            int delta = std::numeric_limits<int>::min();
            for(int v : val)
               delta = std::max(delta,v);
            out.setInt(maxWup, cUp[maxWup] + delta);
         }
      });

      mdd.transitionDown(len,{len},{},[len](auto& out,const auto& pDown,const auto& pCombined,const auto& var, const auto& val,bool up) {
         out.setInt(len,pDown[len] + 1);
      });
      mdd.transitionUp(lenUp,{lenUp},{},[lenUp](auto& out,const auto& cUp,const auto& cCombined,const auto& var, const auto& val,bool up) {
         out.setInt(lenUp,cUp[lenUp] + 1);
      });
      mdd.onFixpoint([z,minW,maxW](const auto& sinkDown,const auto& sinkUp,const auto& sinkCombined) {
         z->updateBounds(sinkDown.at(minW),sinkDown.at(maxW));
      });
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
      const int minW = mdd.addDownState(d, 0, INT_MAX,MinFun);
      const int maxW = mdd.addDownState(d, 0, INT_MAX,MaxFun);
      const int minWup = mdd.addUpState(d, 0, INT_MAX,MinFun);
      const int maxWup = mdd.addUpState(d, 0, INT_MAX,MaxFun);
      // State 'len' is needed to capture the index i, to express array[i]*val when vars[i]=val.
      const int len  = mdd.addDownState(d, 0, vars.size(),MaxFun);
      const int lenUp  = mdd.addUpState(d, 0, vars.size(),MaxFun);

      mdd.arcExist(d,[=] (const auto& pDown, const auto& pCombined, const auto& cUp, const auto& cCombined, var<int>::Ptr var, const auto& val,bool upPass) -> bool {
         if (upPass==true) {
            return ((pDown[minW] + val*array[pDown[len]] + cUp[minWup] <= z->max()) &&
                    (pDown[maxW] + val*array[pDown[len]] + cUp[maxWup] >= z->min()));
         } else {
            return ((pDown[minW] + val*array[pDown[len]] + Lproxy[pDown[len]] <= z->max()) &&
                    (pDown[maxW] + val*array[pDown[len]] + Uproxy[pDown[len]] >= z->min()));
         }
      });

      mdd.transitionDown(minW,{len,minW},{},[minW,array,len] (auto& out,const auto& pDown,const auto& pCombined,const auto& var, const auto& val,bool up) {
         int delta = std::numeric_limits<int>::max();
         auto coef = array[pDown[len]];
         for(int v : val)
            delta = std::min(delta,coef * v);
         out.setInt(minW,pDown[minW] + delta);
      });
      mdd.transitionDown(maxW,{len,maxW},{},[maxW,array,len] (auto& out,const auto& pDown,const auto& pCombined,const auto& var, const auto& val,bool up) {
         int delta = std::numeric_limits<int>::min();
         auto coef = array[pDown[len]];
         for(int v : val)
            delta = std::max(delta,coef*v);
         out.setInt(maxW, pDown[maxW] + delta);
      });

      mdd.transitionUp(minWup,{lenUp,minWup},{},[nbVars,minWup,array,lenUp] (auto& out,const auto& cUp,const auto& cCombined,const auto& var, const auto& val,bool up) {
         if (cUp[lenUp] < nbVars) {
            int delta = std::numeric_limits<int>::max();
            auto coef = array[nbVars - cUp[lenUp]-1];
            for(int v : val)
               delta = std::min(delta,coef*v);
            out.setInt(minWup, cUp[minWup] + delta);
         }
      });
      mdd.transitionUp(maxWup,{lenUp,maxWup},{},[nbVars,maxWup,array,lenUp] (auto& out,const auto& cUp,const auto& cCombined,const auto& var, const auto& val,bool up) {
         if (cUp[lenUp] < nbVars) {
            int delta = std::numeric_limits<int>::min();
            auto coef = array[nbVars - cUp[lenUp]-1];
            for(int v : val)
               delta = std::max(delta,coef*v);
            out.setInt(maxWup, cUp[maxWup] + delta);
         }
      });

      mdd.transitionDown(len,{len},{},[len](auto& out,const auto& pDown,const auto& pCombined,const auto& var, const auto& val,bool up) {
         out.setInt(len,pDown[len] + 1);
      });
      mdd.transitionUp(lenUp,{lenUp},{},[lenUp](auto& out,const auto& cUp,const auto& cCombined,const auto& var, const auto& val,bool up) {
         out.setInt(lenUp,cUp[lenUp] + 1);
      });

      mdd.onFixpoint([z,minW,maxW](const auto& sinkDown,const auto& sinkUp,const auto& sinkCombined) {
         z->updateBounds(sinkDown.at(minW),sinkDown.at(maxW));
      });
   }

   void sumMDD(MDDSpec& mdd, const Factory::Veci& vars, const std::vector<std::vector<int>>& matrix, var<int>::Ptr z) {
      // Enforce MDD bounds consistency on
      //   sum(i, matrix[i][vars[i]]) == z
      mdd.append(vars);
      mdd.addGlobal(std::array<var<int>::Ptr,1>{z});

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
      const int minW = mdd.addDownState(d, 0, INT_MAX,MinFun);
      const int maxW = mdd.addDownState(d, 0, INT_MAX,MaxFun);
      const int minWup = mdd.addUpState(d, 0, INT_MAX,MinFun);
      const int maxWup = mdd.addUpState(d, 0, INT_MAX,MaxFun);
      // State 'len' is needed to capture the index i, to express matrix[i][vars[i]]
      const int len  = mdd.addDownState(d, 0, vars.size(),MaxFun);
      const int lenUp  = mdd.addUpState(d, 0, vars.size(),MaxFun);

      mdd.arcExist(d,[=] (const auto& pDown, const auto& pCombined, const auto& cUp, const auto& cCombined, var<int>::Ptr var, const auto& val,bool upPass) -> bool {
         const int mlv = matrix[pDown[len]][val];
         if (upPass==true) {
            return ((pDown[minW] + mlv + cUp[minWup] <= z->max()) &&
                    (pDown[maxW] + mlv + cUp[maxWup] >= z->min()));
         } else {
            return ((pDown[minW] + mlv + Lproxy[pDown[len]] <= z->max()) && 
                    (pDown[maxW] + mlv + Uproxy[pDown[len]] >= z->min()));
         }
      });

      mdd.transitionDown(minW,{len,minW},{},[minW,matrix,len] (auto& out,const auto& pDown,const auto& pCombined,const auto& var, const auto& val,bool up) {
         int delta = std::numeric_limits<int>::max();
         const auto& row = matrix[pDown[len]];
         for(int v : val)
            delta = std::min(delta,row[v]);
         out.setInt(minW,pDown[minW] + delta);
      });
      mdd.transitionDown(maxW,{len,maxW},{},[maxW,matrix,len] (auto& out,const auto& pDown,const auto& pCombined,const auto& var,const auto& val,bool up) {
         int delta = std::numeric_limits<int>::min();
         const auto& row = matrix[pDown[len]];
         for(int v : val)
            delta = std::max(delta,row[v]);
         out.setInt(maxW,pDown[maxW] + delta);
      });
      mdd.transitionUp(minWup,{lenUp,minWup},{},[nbVars,minWup,matrix,lenUp] (auto& out,const auto& cUp,const auto& cCombined,const auto& var,const auto& val,bool up) {
         if (cUp[lenUp] < nbVars) {
            int delta = std::numeric_limits<int>::max();
            const auto& row = matrix[nbVars - cUp[lenUp]-1];
            for(int v : val)
               delta = std::min(delta,row[v]);
            out.setInt(minWup, cUp[minWup] + delta);
         }
      });
      mdd.transitionUp(maxWup,{lenUp,maxWup},{},[nbVars,maxWup,matrix,lenUp] (auto& out,const auto& cUp,const auto& cCombined,const auto& var,const auto& val,bool up) {
         if (cUp[lenUp] < nbVars) {
            int delta = std::numeric_limits<int>::min();
            const auto& row = matrix[nbVars - cUp[lenUp]-1];
            for(int v : val)
               delta = std::max(delta,row[v]);
            out.setInt(maxWup, cUp[maxWup] + delta);
         }
      });

      mdd.transitionDown(len,{len},{},[len](auto& out,const auto& pDown,const auto& pCombined,const auto& var, const auto& val,bool up) {
         out.setInt(len,  pDown[len] + 1);
      });
      mdd.transitionUp(lenUp,{lenUp},{},[lenUp](auto& out,const auto& cUp,const auto& cCombined,const auto& var, const auto& val,bool up) {
         out.setInt(lenUp,  cUp[lenUp] + 1);
      });

      mdd.onFixpoint([z,minW,maxW](const auto& sinkDown,const auto& sinkUp,const auto& sinkCombined) {
         z->updateBounds(sinkDown.at(minW),sinkDown.at(maxW));
      });

      mdd.splitOnLargest([minW](const auto& in) { return in.getDownState()[minW];});
   }
}
