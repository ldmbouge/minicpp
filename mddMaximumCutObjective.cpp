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

   MDDCstrDesc::Ptr maxCutObjectiveMDD(MDD::Ptr m, const Factory::Vecb& vars,
                                       const std::vector<std::vector<int>>& weights,
                                       var<int>::Ptr z,MDDOpts opts)
   {
      MDDSpec& mdd = m->getSpec();
      mdd.addGlobal(std::array<var<int>::Ptr,1>{z});
      int nbVars = (int)vars.size();
      auto d = mdd.makeConstraintDescriptor(vars,"maximumCutObjectiveMDD");

      // rootValue is sum of negative weights
      int rootValue = 0;
      for (int i = 0; i < nbVars; i++)
         for (int j = i + 1; j < nbVars; j++)
            if (weights[i][j] < 0) rootValue += weights[i][j];

      // Define the states
      const int downWeights = mdd.addDownSWState(d, nbVars, 0, 0, External, opts.cstrP);
      const int maxDownValue = mdd.addDownState(d, rootValue, INT_MAX, External, opts.cstrP);
      const int len = mdd.addDownState(d, 0, nbVars, MaxFun, opts.cstrP);

      mdd.arcExist(d,[=](const auto&,const auto&,var<int>::Ptr, const auto&) -> bool {
         return true;
      });

      mdd.transitionDown(d,downWeights,{downWeights,maxDownValue},{},[=] (auto& out,const auto& parent,const auto& var, const auto& val) {
         bool relaxed = val.size()==2;
         int k = parent.down[len];
         MDDSWin<short> outWeights = out.getSW(downWeights);
         MDDSWin<short> parentWeights = parent.down.getSW(downWeights);
         for (int i = 0; i <= k; i++) outWeights.set(i, 0);

         if (!relaxed) {
            bool inS = val.contains(false);
            for (int i = k + 1; i < nbVars; i++) {
               outWeights.set(i, parentWeights.get(i) + (inS ? weights[i][k] : - weights[i][k]));
            }
         } else {
            int inSWeight, inTWeight;
            for (int i = k + 1; i < nbVars; i++) {
               inSWeight = parentWeights.get(i) + weights[i][k];
               inTWeight = parentWeights.get(i) - weights[i][k];
               if (inSWeight >= 0 && inTWeight >= 0) outWeights.set(i, std::min(inSWeight, inTWeight));
               else if (inSWeight <= 0 && inTWeight <= 0) outWeights.set(i, std::max(inSWeight, inTWeight));
               else outWeights.set(i, 0);
            }
         }
      });
      mdd.transitionDown(d,maxDownValue,{downWeights,maxDownValue},{},[=] (auto& out,const auto& parent,const auto& var,const auto& val) {
         bool relaxed = val.size()==2;
         int k = parent.down[len];
         MDDSWin<short> parentWeights = parent.down.getSW(downWeights);

         if (!relaxed) {
            bool inS = val.contains(false);
            int newValue = std::max(0, inS ? -parentWeights.get(k) : parentWeights.get(k));
            for (int i = k + 1; i < nbVars; i++) {
               if ((inS && parentWeights.get(i) * weights[i][k] <= 0) ||
                   (!inS && parentWeights.get(i) * weights[i][k] >= 0)) {
                  newValue += std::min(abs(parentWeights.get(i)), abs(weights[i][k]));
               }
            }
            out.setInt(maxDownValue, parent.down[maxDownValue] + newValue);
         } else {
            int newValueForS = std::max(0, -parentWeights.get(k));
            int newValueForT = std::max(0, (int)parentWeights.get(k));
            for (int i = k + 1; i < nbVars; i++) {
               if (parentWeights.get(i) * weights[i][k] <= 0) newValueForS += std::min(abs(parentWeights.get(i)), abs(weights[i][k]));
               else newValueForT += std::min(abs(parentWeights.get(i)), abs(weights[i][k]));

               int inSWeight = parentWeights.get(i) + weights[i][k];
               int inTWeight = parentWeights.get(i) - weights[i][k];
               int relaxedWeight;
               if (inSWeight >= 0 && inTWeight >= 0) relaxedWeight = std::min(inSWeight, inTWeight);
               else if (inSWeight <= 0 && inTWeight <= 0) relaxedWeight = std::max(inSWeight, inTWeight);
               else relaxedWeight = 0;

               newValueForS += abs(inSWeight) - abs(relaxedWeight);
               newValueForT += abs(inTWeight) - abs(relaxedWeight);
            }

            out.setInt(maxDownValue, parent.down[maxDownValue] + std::max(newValueForS, newValueForT));
         }
      });
      mdd.transitionDown(d,len,{len},{},[len] (auto& out,const auto& parent,const auto&,const auto&) {
         out.setInt(len,parent.down[len] + 1);
      });

      mdd.addRelaxationDown(d,downWeights,[](auto& out,const auto& l,const auto& r) noexcept {});
      mdd.addRelaxationDown(d,maxDownValue,[=](auto& out,const auto& l,const auto& r) noexcept    {
         MDDSWin<short> outWeights = out.getSW(downWeights);
         MDDSWin<short> lWeights = l.getSW(downWeights);
         MDDSWin<short> rWeights = r.getSW(downWeights);
         int lValue = l[maxDownValue];
         int rValue = r[maxDownValue];
         int k = l[len];
         int i;
         for (i = 0; i < k; i++) outWeights.set(i, 0);
         for (; i < nbVars; i++) {
            int lWeight = lWeights.get(i);
            int rWeight = rWeights.get(i);
            int relaxedWeight;
            if (lWeight >= 0 && rWeight >=0) relaxedWeight = std::min(lWeight, rWeight);
            else if (lWeight <= 0 && rWeight <=0) relaxedWeight = std::max(lWeight, rWeight);
            else relaxedWeight = 0;
            outWeights.set(i, relaxedWeight);
            lValue += abs(lWeight) - abs(relaxedWeight);
            rValue += abs(rWeight) - abs(relaxedWeight);
         }
         out.setInt(maxDownValue, std::max(lValue, rValue));
      });

      mdd.onFixpoint([z,maxDownValue](const auto& sinkDown,const auto& sinkUp,const auto& sinkCombined) {
         z->removeAbove(sinkDown[maxDownValue]);
      });

      switch (opts.candP) {
        case 0:
          mdd.candidateByLargest([maxDownValue](const auto& state, void* arcs, int numArcs) {
             return state[maxDownValue];
          }, opts.cstrP);
          break;
        case 1:
          mdd.candidateByLargest([downWeights,maxDownValue,len,nbVars](const auto& state, void* arcs, int numArcs) {
               int rank = state[maxDownValue];
               MDDSWin<short> stateWeights = state.getSW(downWeights);
               for (int i = state[len]; i < nbVars; i++) {
                 rank += abs(stateWeights.get(i));
               }
               return rank;
          }, opts.cstrP);
          break;
        default:
          break;
      }
      return d;
   }
}
