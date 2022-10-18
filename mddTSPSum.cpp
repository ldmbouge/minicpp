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
#include <algorithm>
#include <numeric>

namespace Factory {
   MDDCstrDesc::Ptr tspSumMDD(MDD::Ptr m, const Factory::Veci& vars, const std::vector<std::vector<int>>& matrix, var<int>::Ptr z, Objective::Ptr objective, MDDOpts opts) {
      MDDSpec& mdd = m->getSpec();
      mdd.addGlobal({z});
      auto d = mdd.makeConstraintDescriptor(vars,"sumMDD");
      auto udom = domRange(vars);
      int minDom = udom.first;
      const int domSize = udom.second - udom.first + 1;

      // Define the states
      const auto minW = mdd.downIntState(d, 0, INT_MAX,MinFun,opts.cstrP,false);
      const auto maxW = mdd.downIntState(d, 0, INT_MAX,MaxFun,opts.cstrP,false);
      const auto minWup = mdd.upIntState(d, 0, INT_MAX,MinFun);
      const auto maxWup = mdd.upIntState(d, 0, INT_MAX,MaxFun);
      const auto prev  = mdd.downBSState(d,domSize,0,External,opts.cstrP,true);
      const auto next  = mdd.upBSState(d,domSize,0,External);
      const auto firstDown = mdd.downIntState(d, 1, 1, MinFun,opts.cstrP,false);
      const auto firstUp = mdd.upIntState(d, 1, 1, MinFun);

      mdd.arcExist(d,[=] (const auto& parent,const auto& child,var<int>::Ptr var, const auto& val) {
         if (child.up.unused()) {
            if (parent.down[firstDown]) return true;
            int minArcWeightFromPrev = std::numeric_limits<int>::max();
            auto previous = parent.down[prev];
            for (int i = 0; i < domSize; i++)
               if (previous.getBit(i))
                  minArcWeightFromPrev = std::min(minArcWeightFromPrev, matrix[i][val]);
            return parent.down[minW] + minArcWeightFromPrev <= z->max();
         }
         if (parent.down[firstDown] || child.up[firstUp]) return true;
         int minArcWeightFromPrev = std::numeric_limits<int>::max();
         int maxArcWeightFromPrev = std::numeric_limits<int>::min();
         int minArcWeightFromNext = std::numeric_limits<int>::max();
         int maxArcWeightFromNext = std::numeric_limits<int>::min();
         auto previous = parent.down[prev];
         auto nextVals = child.up[next];
         for (int i = 0; i < domSize; i++) {
            if (previous.getBit(i)) {
               minArcWeightFromPrev = std::min(minArcWeightFromPrev, matrix[i][val]);
               maxArcWeightFromPrev = std::max(maxArcWeightFromPrev, matrix[i][val]);
            }
            if (nextVals.getBit(i)) {
               minArcWeightFromNext = std::min(minArcWeightFromNext, matrix[val][i]);
               maxArcWeightFromNext = std::max(maxArcWeightFromNext, matrix[val][i]);
            }
         }
         return ((parent.down[minW] + minArcWeightFromPrev + minArcWeightFromNext + child.up[minWup] <= z->max()) &&
                 (parent.down[maxW] + maxArcWeightFromPrev + maxArcWeightFromNext + child.up[maxWup] >= z->min()));
      });
      mdd.nodeExist([=](const auto& n) {
        if (n.down[firstDown] || n.up[firstUp]) return true;
        int minWeightConnection = std::numeric_limits<int>::max();
        int maxWeightConnection = std::numeric_limits<int>::min();
        auto previous = n.down[prev];
        auto nextVals = n.up[next];
        for (int i = 0; i < domSize; i++) {
           if (previous.getBit(i)) {
              for (int j = 0; j < domSize; j++) {
                 if (nextVals.getBit(j)) {
                    minWeightConnection = std::min(minWeightConnection, matrix[i][j]);
                    maxWeightConnection = std::max(maxWeightConnection, matrix[i][j]);
                 }
              }
           }
        }
        return (n.down[minW] + minWeightConnection + n.up[minWup] <= z->max()) && (n.down[maxW] + maxWeightConnection + n.up[maxWup] >= z->min());
      });


      mdd.transitionDown(d,minW,{minW,prev},{},[domSize,minW,matrix,prev,firstDown] (auto& out,const auto& parent,const auto&, const auto& val) {
         if (!parent.down[firstDown]) {
            int minArcWeight = std::numeric_limits<int>::max();
            auto previous = parent.down[prev];
            for (int i = 0; i < domSize; i++)
               if (previous.getBit(i))
                  for (int v : val)
                     minArcWeight = std::min(minArcWeight, matrix[i][v]);
            out[minW] = parent.down[minW] + minArcWeight;
         }
      });
      mdd.transitionDown(d,maxW,{maxW,prev},{},[domSize,maxW,matrix,prev,firstDown] (auto& out,const auto& parent,const auto&,const auto& val) {
         if (!parent.down[firstDown]) {
            int maxArcWeight = std::numeric_limits<int>::min();
            auto previous = parent.down[prev];
            for (int i = 0; i < domSize; i++)
               if (previous.getBit(i))
                  for (int v : val)
                     maxArcWeight = std::max(maxArcWeight, matrix[i][v]);
            out[maxW] = parent.down[maxW] + maxArcWeight;
         }
      });
      mdd.transitionUp(d,minWup,{minWup,next},{},[domSize,minWup,matrix,next,firstUp](auto& out,const auto& child,const auto&,const auto& val) {
         if (!child.up[firstUp]) {
            int minArcWeight = std::numeric_limits<int>::max();
            auto nextVals = child.up[next];
            for (int i = 0; i < domSize; i++)
               if (nextVals.getBit(i))
                  for (int v : val)
                     minArcWeight = std::min(minArcWeight, matrix[v][i]);
            out[minWup] = child.up[minWup] + minArcWeight;
         }
      });
      mdd.transitionUp(d,maxWup,{maxWup,next},{},[domSize,maxWup,matrix,next,firstUp](auto& out,const auto& child,const auto&,const auto& val) {
         if (!child.up[firstUp]) {
            int maxArcWeight = std::numeric_limits<int>::min();
            auto nextVals = child.up[next];
            for (int i = 0; i < domSize; i++)
               if (nextVals.getBit(i))
                  for (int v : val)
                     maxArcWeight = std::max(maxArcWeight, matrix[v][i]);
            out[maxWup] = child.up[maxWup] + maxArcWeight;
         }
      });

      mdd.transitionDown(d,prev,{prev},{},[minDom,prev](auto& out,const auto& parent,const auto&, const auto& val) {
         MDDBSValue sv(out[prev]);
         for(auto v : val)
            sv.set(v - minDom);
      });
      mdd.transitionUp(d,next,{next},{},[minDom,next](auto& out,const auto& child,const auto&, const auto& val) {
         MDDBSValue sv(out[next]);
         for(auto v : val)
            sv.set(v - minDom);
      });

      mdd.transitionDown(d,firstDown,{firstDown},{},[firstDown](auto& out,const auto& parent,const auto&, const auto& val) {
         out[firstDown] = 0;
      });
      mdd.transitionUp(d,firstUp,{firstUp},{},[firstUp](auto& out,const auto& child,const auto&, const auto& val) {
         out[firstUp] = 0;
      });

      mdd.addRelaxationDown(d,prev,[prev](auto& out,const auto& l,const auto& r) noexcept  {
         out[prev].setBinOR(l[prev],r[prev]);
      });
      mdd.addRelaxationUp(d,next,[next](auto& out,const auto& l,const auto& r) noexcept  {
         out[next].setBinOR(l[next],r[next]);
      });

      mdd.onFixpoint([z,minW,maxW](const auto& sink) {
         z->updateBounds(sink.down[minW],sink.down[maxW]);
      });
      if (objective) {
         if (objective->isMin()) {
            mdd.onRestrictedFixpoint([objective,minW,maxW](const auto& sink) {
               objective->foundPrimal(sink.down[minW]);
            });
         } else {
            mdd.onRestrictedFixpoint([objective,maxW](const auto& sink) {
               objective->foundPrimal(sink.down[maxW]);
            });
         }
      }

      mdd.splitOnLargest([minW](const auto& in) { return -in.getDownState()[minW];});

      std::vector<int> valueOrdering;
      std::vector<int> numPrecedence;
      std::vector<int> averageDistance;
      for (int i = 0; i < domSize; i++) {
         valueOrdering.push_back(i);

         int precedenceCount = 0;
         int sumOfDistance = 0;
         int numConnections = 0;
         for (int j = 0; j < domSize; j++) {
            if (i == j) continue;
            if (matrix[j][i] < 0) precedenceCount++;
            else {
               sumOfDistance += matrix[j][i];
               numConnections++;
            }
            if (matrix[i][j] >= 0) {
               sumOfDistance += matrix[i][j];
               numConnections++;
            }
         }
         numPrecedence.push_back(precedenceCount);
         averageDistance.push_back(sumOfDistance/numConnections);
      }
      std::sort(valueOrdering.begin(), valueOrdering.end(), [=](int a, int b) {
         return numPrecedence[a] > numPrecedence[b] || (numPrecedence[a] == numPrecedence[b] && averageDistance[a] > averageDistance[b]);
      });

      std::vector<int> valueWeights(domSize);
      int weight = domSize;
      for (auto value : valueOrdering) valueWeights[value] = weight--;

      mdd.candidateByLargest([minW](const auto& state, void* arcs, int numArcs) {
         return -state[minW];
      });
//      mdd.candidateByLargest([valueWeights,prev,domSize](const auto& state, void* arcs, int numArcs) {
//         auto previous = state[prev];
//         for (int i = 0; i < domSize; i++) {
//            if (previous.getBit(i)) {
//               return valueWeights[i];
//            }
//         }
//         return 0;
//      });

      mdd.bestValue([=](auto layer) {
         int bestValue = 0;
         int bestWeight = 0;
         for (auto& node : *layer) {
            for (auto& childArc : node->getChildren()) {
               auto child = childArc->getChild();
               //int childWeight = child->getDownState()[maxW];
               int childWeight = child->getDownState()[minW];
               //int childWeight = child->getDownState()[maxW] + child->getUpState()[maxWup];
               //int childWeight = child->getDownState()[minW] + child->getUpState()[minWup];
               if (childWeight > bestWeight) {
                  bestWeight = childWeight;
                  bestValue = childArc->getValue();
               }
            }
         }
         return bestValue;
      });

      return d;
   }
}
