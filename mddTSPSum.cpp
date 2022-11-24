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
   MDDCstrDesc::Ptr tspSumMDD(MDD::Ptr m, const Factory::Veci& vars, const std::vector<std::vector<int>>& matrix, MDDPBitSequence::Ptr all, MDDPBitSequence::Ptr allup, var<int>::Ptr z, Objective::Ptr objective, MDDOpts opts) {
      MDDSpec& mdd = m->getSpec();
      mdd.addGlobal({z});
      auto d = mdd.makeConstraintDescriptor(vars,"sumMDD");
      auto udom = domRange(vars);
      int minDom = udom.first;
      const int domSize = udom.second - udom.first + 1;
      const int numVars = (int)vars.size();

      std::vector<std::vector<std::pair<int, int>>> sortedIncomingConnections(domSize);
      std::vector<std::vector<std::pair<int, int>>> sortedOutgoingConnections(domSize);
      for (int i = 0; i < domSize; i++) {
         auto row = matrix[i];
         for (int j = 0; j < domSize; j++) {
            if (i == j || matrix[i][j] < 0) continue;
            sortedIncomingConnections[j].push_back(std::make_pair(i,row[j]));
            sortedOutgoingConnections[i].push_back(std::make_pair(j,row[j]));
         }
      }
      for (int i = 0; i < domSize; i++) {
         sort(sortedIncomingConnections[i].begin(), sortedIncomingConnections[i].end(), [=](const std::pair<int,int> &a, const std::pair<int, int> &b) { return a.second < b.second; });
         sort(sortedOutgoingConnections[i].begin(), sortedOutgoingConnections[i].end(), [=](const std::pair<int,int> &a, const std::pair<int, int> &b) { return a.second < b.second; });
      }

      // Define the states
      const auto minW = mdd.downIntState(d, 0, INT_MAX,MinFun,opts.cstrP,false);
      const auto maxW = mdd.downIntState(d, 0, INT_MAX,MaxFun,opts.cstrP,false);
      const auto minWup = mdd.upIntState(d, 0, INT_MAX,MinFun);
      const auto maxWup = mdd.upIntState(d, 0, INT_MAX,MaxFun);
      const auto prev  = mdd.downBSState(d,domSize,0,External,opts.cstrP);
      const auto next  = mdd.upBSState(d,domSize,0,External);
      const auto len   = mdd.downIntState(d,0,vars.size(),MinFun,opts.cstrP);
      const auto lenup   = mdd.upIntState(d,0,vars.size(),MinFun,opts.cstrP);


      //std::vector<int>* possibleValues = new std::vector<int>();
      int* possibleValues = (int*)malloc(numVars * sizeof(int));
      mdd.arcExist(d,[=] (const auto& parent,const auto& child,var<int>::Ptr var, const auto& val) {
         if (child.up.unused()) {
            if (parent.down[len] == 0) return true;
            int minArcWeightFromPrev = std::numeric_limits<int>::max();
            auto previous = parent.down[prev];
            for (auto p : sortedIncomingConnections[val]) {
               if (previous.getBit(p.first)) {
                  minArcWeightFromPrev = p.second;
                  break;
               }
            }
            int lowerBoundBelow = 0;
            if (all) {
               for (int i = 0; i < domSize; i++) {
                  if (i != val && !parent.down[all].getBit(i)) {
                     for (auto connection : sortedIncomingConnections[i]) {
                        if (!parent.down[all].getBit(connection.first)) {
                           lowerBoundBelow += connection.second;
                           break;
                        }
                     }
                  }
               }
            }
            return parent.down[minW] + minArcWeightFromPrev + lowerBoundBelow <= z->max();
         }
         if (parent.down[len] == 0 || child.up[lenup] == 0) return true;
         int minArcWeightFromPrev = std::numeric_limits<int>::max();
         //int maxArcWeightFromPrev = std::numeric_limits<int>::min();
         int minArcWeightFromNext = std::numeric_limits<int>::max();
         //int maxArcWeightFromNext = std::numeric_limits<int>::min();
         auto previous = parent.down[prev];
         auto nextVals = child.up[next];
         for (auto p : sortedIncomingConnections[val]) {
            if (previous.getBit(p.first)) {
               minArcWeightFromPrev = p.second;
               break;
            }
         }
         //for (auto p = sortedIncomingConnections[val].rbegin(); p != sortedIncomingConnections[val].rend(); ++p) {
         //   if (previous.getBit(p->first)) {
         //      maxArcWeightFromPrev = p->second;
         //      break;
         //   }
         //}
         for (auto p : sortedOutgoingConnections[val]) {
            if (nextVals.getBit(p.first)) {
               minArcWeightFromNext = p.second;
               break;
            }
         }
         //for (auto p = sortedOutgoingConnections[val].rbegin(); p != sortedOutgoingConnections[val].rend(); ++p) {
         //   if (nextVals.getBit(p->first)) {
         //      maxArcWeightFromNext = p->second;
         //      break;
         //   }
         //}
         int lowerBoundBelow = 0;
         int lowerBoundAbove = 0;
         if (all) {
            auto parentAll = parent.down[all];
            int numValues = 0;
            //possibleValues->clear();
            for (int i = 0; i < domSize; i++) {
               if (i != val && !parentAll.getBit(i)) {
                  for (auto connection : sortedIncomingConnections[i]) {
                     if (!parentAll.getBit(connection.first)) {
                        //possibleValues->push_back(connection.second);
                        possibleValues[numValues++] = connection.second;
                        break;
                     }
                  }
               }
            }
            //std::sort(possibleValues->begin(), possibleValues->end());
            std::sort(possibleValues, possibleValues + numValues);
            for (int i = 0; i < child.up[lenup] - 1; i++) {
               //lowerBoundBelow += (*possibleValues)[i];
               lowerBoundBelow += possibleValues[i];
            }
         }
         //if (allup) {
         //   std::vector<int> possibleValues;
         //   for (int i = 0; i < domSize; i++) {
         //      if (i != val && !child.up[allup].getBit(i)) {
         //         for (auto connection : sortedOutgoingConnections[i]) {
         //            if (!child.up[allup].getBit(connection.first)) {
         //               possibleValues.push_back(connection.second);
         //               break;
         //            }
         //         }
         //      }
         //   }
         //   std::sort(possibleValues.begin(), possibleValues.end());
         //   for (int i = 0; i < parent.down[len] - 1; i++) {
         //      lowerBoundAbove += possibleValues[i];
         //   }
         //}
         lowerBoundBelow = std::max(lowerBoundBelow,(int)child.up[minWup]);
         lowerBoundAbove = std::max(lowerBoundAbove,(int)parent.down[minW]);
         return ((lowerBoundAbove + minArcWeightFromPrev + minArcWeightFromNext + lowerBoundBelow <= z->max()));// &&
//                 (parent.down[maxW] + maxArcWeightFromPrev + maxArcWeightFromNext + child.up[maxWup] >= z->min()));
      });

      //mdd.arcExist(d,[=] (const auto& parent,const auto& child,var<int>::Ptr var, const auto& val) {
      //   if (parent.down[len] == 0) return true;
      //   int minArcWeightFromPrev = std::numeric_limits<int>::max();
      //   auto previous = parent.down[prev];
      //   for (auto p : sortedIncomingConnections[val]) {
      //      if (previous.getBit(p.first)) {
      //         minArcWeightFromPrev = p.second;
      //         break;
      //      }
      //   }
      //   int lowerBoundBelow = 0;
      //   auto parentAll = parent.down[all];
      //   if (all) {
      //      if (child.up.unused()) {
      //         for (int i = 0; i < domSize; i++) {
      //            if (i != val && !parentAll.getBit(i)) {
      //               for (auto connection : sortedIncomingConnections[i]) {
      //                  if (!parentAll.getBit(connection.first)) {
      //                     lowerBoundBelow += connection.second;
      //                     break;
      //                  }
      //               }
      //            }
      //         }
      //      } else {
      //         int numValues = 0;
      //         //possibleValues->clear();
      //         for (int i = 0; i < domSize; i++) {
      //            if (i != val && !parentAll.getBit(i)) {
      //               for (auto connection : sortedIncomingConnections[i]) {
      //                  if (!parentAll.getBit(connection.first)) {
      //                     possibleValues[numValues++] = connection.second;
      //                     //possibleValues->push_back(connection.second);
      //                     break;
      //                  }
      //               }
      //            }
      //         }
      //         //std::sort(possibleValues->begin(), possibleValues->end());
      //         std::sort(possibleValues, possibleValues + numValues);
      //         for (int i = numVars - parent.down[len] - 2; i >= 0; i--) {
      //            lowerBoundBelow += possibleValues[i];
      //            //lowerBoundBelow += (*possibleValues)[i];
      //         }
      //      }
      //   }
      //   return parent.down[minW] + minArcWeightFromPrev + lowerBoundBelow <= z->max();
      //});
      mdd.nodeExist([=](const auto& n) {
         if (n.down[len] == 0 || n.up[lenup] == 0) return true;
        int minWeightConnection = std::numeric_limits<int>::max();
        int maxWeightConnection = std::numeric_limits<int>::min();
        auto previous = n.down[prev];
        auto nextVals = n.up[next];
        for (int i = 0; i < domSize; i++) {
           if (previous.getBit(i)) {
              for (int j = 0; j < domSize; j++) {
                 if (nextVals.getBit(j)) {
                    if (i != j && matrix[i][j] >= 0) {
                       minWeightConnection = std::min(minWeightConnection, matrix[i][j]);
                       maxWeightConnection = std::max(maxWeightConnection, matrix[i][j]);
                    }
                 }
              }
           }
        }
        return (n.down[minW] + minWeightConnection + n.up[minWup] <= z->max()) && (n.down[maxW] + maxWeightConnection + n.up[maxWup] >= z->min());
      });


      mdd.transitionDown(d,minW,{minW,prev},{},[domSize,minW,matrix,prev,len] (auto& out,const auto& parent,const auto&, const auto& val) {
         if (parent.down[len]) {
            int minArcWeight = std::numeric_limits<int>::max();
            auto previous = parent.down[prev];
            for (int i = 0; i < domSize; i++)
               if (previous.getBit(i))
                  for (int v : val)
                     if (i != v && matrix[i][v] >= 0)
                        minArcWeight = std::min(minArcWeight, matrix[i][v]);
            out[minW] = parent.down[minW] + minArcWeight;
         }
      });
      mdd.transitionDown(d,maxW,{maxW,prev},{},[domSize,maxW,matrix,prev,len] (auto& out,const auto& parent,const auto&,const auto& val) {
         if (parent.down[len]) {
            int maxArcWeight = std::numeric_limits<int>::min();
            auto previous = parent.down[prev];
            for (int i = 0; i < domSize; i++)
               if (previous.getBit(i))
                  for (int v : val)
                     if (i != v && matrix[i][v] >= 0)
                        maxArcWeight = std::max(maxArcWeight, matrix[i][v]);
            out[maxW] = parent.down[maxW] + maxArcWeight;
         }
      });
      mdd.transitionUp(d,minWup,{minWup,next},{},[domSize,minWup,matrix,next,lenup](auto& out,const auto& child,const auto&,const auto& val) {
         if (child.up[lenup]) {
            int minArcWeight = std::numeric_limits<int>::max();
            auto nextVals = child.up[next];
            for (int i = 0; i < domSize; i++)
               if (nextVals.getBit(i))
                  for (int v : val)
                     if (i != v && matrix[v][i] >= 0)
                        minArcWeight = std::min(minArcWeight, matrix[v][i]);
            out[minWup] = child.up[minWup] + minArcWeight;
         }
      });
      mdd.transitionUp(d,maxWup,{maxWup,next},{},[domSize,maxWup,matrix,next,lenup](auto& out,const auto& child,const auto&,const auto& val) {
         if (child.up[lenup]) {
            int maxArcWeight = std::numeric_limits<int>::min();
            auto nextVals = child.up[next];
            for (int i = 0; i < domSize; i++)
               if (nextVals.getBit(i))
                  for (int v : val)
                     if (i != v && matrix[v][i] >= 0)
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


      mdd.transitionDown(d,len,{len},{},[len](auto& out,const auto& in,const auto&,const auto& val) noexcept {
         out[len] = in.down[len] + 1;
      });
      mdd.transitionUp(d,lenup,{lenup},{},[lenup](auto& out,const auto& in,const auto&,const auto& val) noexcept {
         out[lenup] = in.up[lenup] + 1;
      });

      mdd.addRelaxationDown(d,prev,[prev](auto& out,const auto& l,const auto& r) noexcept  {
         out[prev].setBinOR(l[prev],r[prev]);
      });
      mdd.addRelaxationUp(d,next,[next](auto& out,const auto& l,const auto& r) noexcept  {
         out[next].setBinOR(l[next],r[next]);
      });

      mdd.onFixpoint([z,minW,maxW,minWup,maxWup,len,lenup,prev,next,matrix,domSize](const auto& sink) {
         if (sink.down[len] == 0) {
            z->updateBounds(sink.up[minWup],sink.up[maxWup]);
         } else if (sink.up[lenup] == 0) {
            z->updateBounds(sink.down[minW],sink.down[maxW]);
         } else {
            int minWeightConnection = std::numeric_limits<int>::max();
            int maxWeightConnection = std::numeric_limits<int>::min();
            auto previous = sink.down[prev];
            auto nextVals = sink.up[next];
            for (int i = 0; i < domSize; i++) {
               if (previous.getBit(i)) {
                  for (int j = 0; j < domSize; j++) {
                     if (i != j && nextVals.getBit(j)) {
                        minWeightConnection = std::min(minWeightConnection, matrix[i][j]);
                        maxWeightConnection = std::max(maxWeightConnection, matrix[i][j]);
                     }
                  }
               }
            }
            z->updateBounds(sink.down[minW] + minWeightConnection + sink.up[minWup],sink.down[maxW] + maxWeightConnection + sink.up[maxWup]);
         }
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

      switch (opts.appxEQMode) {
         case 1:
            mdd.equivalenceClassValue([=](const auto& down, const auto& up){
               int minObj = z->min();
               int maxObj = z->max();
               int objRange = maxObj - minObj + 1;
               int blockSize = objRange/4;
               int minWeightConnection = std::numeric_limits<int>::max();
               auto previous = down[prev];
               auto nextVals = up[next];
               for (int i = 0; i < domSize; i++) {
                  if (previous.getBit(i)) {
                     for (int j = 0; j < domSize; j++) {
                        if (nextVals.getBit(j)) {
                           if (i != j && matrix[i][j] >= 0) {
                              minWeightConnection = std::min(minWeightConnection, matrix[i][j]);
                           }
                        }
                     }
                  }
               }
               return (maxObj - (down[minW] + minWeightConnection + up[minWup]))/blockSize;
            }, opts.cstrP);
            break;
         case 2:
            mdd.equivalenceClassValue([=](const auto& down, const auto& up){
               int minObj = z->min();
               int maxObj = z->max();
               int objRange = maxObj - minObj + 1;
               int blockSize = objRange/10;
               int minWeightConnection = std::numeric_limits<int>::max();
               auto previous = down[prev];
               auto nextVals = up[next];
               for (int i = 0; i < domSize; i++) {
                  if (previous.getBit(i)) {
                     for (int j = 0; j < domSize; j++) {
                        if (nextVals.getBit(j)) {
                           if (i != j && matrix[i][j] >= 0) {
                              minWeightConnection = std::min(minWeightConnection, matrix[i][j]);
                           }
                        }
                     }
                  }
               }
               return (maxObj - (down[minW] + minWeightConnection + up[minWup]))/blockSize;
            }, opts.cstrP);
            break;
         default:
            break;
      }

      return d;
   }

   MDDCstrDesc::Ptr tspSumMDD2(MDD::Ptr m, const Factory::Veci& vars, const std::vector<std::vector<int>>& matrix, MDDPBitSequence::Ptr all, MDDPBitSequence::Ptr allup, var<int>::Ptr z, Objective::Ptr objective, MDDOpts opts) {
      MDDSpec& mdd = m->getSpec();
      mdd.addGlobal({z});
      auto d = mdd.makeConstraintDescriptor(vars,"sumMDD");
      auto udom = domRange(vars);
      int minDom = udom.first;
      const int domSize = udom.second - udom.first + 1;
      const int numVars = (int)vars.size();

      std::vector<std::vector<std::pair<int, int>>> sortedIncomingConnections(domSize);
      std::vector<std::vector<std::pair<int, int>>> sortedOutgoingConnections(domSize);
      for (int i = 0; i < domSize; i++) {
         auto row = matrix[i];
         for (int j = 0; j < domSize; j++) {
            if (i == j || matrix[i][j] < 0) continue;
            sortedIncomingConnections[j].push_back(std::make_pair(i,row[j]));
            sortedOutgoingConnections[i].push_back(std::make_pair(j,row[j]));
         }
      }
      for (int i = 0; i < domSize; i++) {
         sort(sortedIncomingConnections[i].begin(), sortedIncomingConnections[i].end(), [=](const std::pair<int,int> &a, const std::pair<int, int> &b) { return a.second < b.second; });
         sort(sortedOutgoingConnections[i].begin(), sortedOutgoingConnections[i].end(), [=](const std::pair<int,int> &a, const std::pair<int, int> &b) { return a.second < b.second; });
      }

      // Define the states
      const auto minW = mdd.downIntState(d, 0, INT_MAX,MinFun,opts.cstrP,false);
      const auto prev  = mdd.downBSState(d,domSize,0,External,opts.cstrP);
      const auto len   = mdd.downIntState(d,0,vars.size(),MinFun,opts.cstrP);


      int* possibleValues = (int*)malloc(numVars * sizeof(int));
      //std::vector<int>* possibleValues = new std::vector<int>();
      mdd.arcExist(d,[=] (const auto& parent,const auto& child,var<int>::Ptr var, const auto& val) {
         if (parent.down[len] == 0) return true;
         int minArcWeightFromPrev = std::numeric_limits<int>::max();
         auto previous = parent.down[prev];
         for (auto p : sortedIncomingConnections[val]) {
            if (previous.getBit(p.first)) {
               minArcWeightFromPrev = p.second;
               break;
            }
         }
         int lowerBoundBelow = 0;
         auto parentAll = parent.down[all];
         if (all) {
            if (child.up.unused()) {
               for (int i = 0; i < domSize; i++) {
                  if (i != val && !parentAll.getBit(i)) {
                     for (auto connection : sortedIncomingConnections[i]) {
                        if (!parentAll.getBit(connection.first)) {
                           lowerBoundBelow += connection.second;
                           break;
                        }
                     }
                  }
               }
            } else {
               int numValues = 0;
               //possibleValues->clear();
               for (int i = 0; i < domSize; i++) {
                  if (i != val && !parentAll.getBit(i)) {
                     for (auto connection : sortedIncomingConnections[i]) {
                        if (!parentAll.getBit(connection.first)) {
                           possibleValues[numValues++] = connection.second;
                           //possibleValues->push_back(connection.second);
                           break;
                        }
                     }
                  }
               }
               //std::sort(possibleValues->begin(), possibleValues->end());
               std::sort(possibleValues, possibleValues + numValues);
               for (int i = numVars - parent.down[len] - 2; i >= 0; i--) {
                  lowerBoundBelow += possibleValues[i];
                  //lowerBoundBelow += (*possibleValues)[i];
               }
            }
         }
         return parent.down[minW] + minArcWeightFromPrev + lowerBoundBelow <= z->max();
      });

      mdd.transitionDown(d,minW,{minW,prev},{},[domSize,minW,matrix,prev,len] (auto& out,const auto& parent,const auto&, const auto& val) {
         if (parent.down[len]) {
            int minArcWeight = std::numeric_limits<int>::max();
            auto previous = parent.down[prev];
            for (int i = 0; i < domSize; i++)
               if (previous.getBit(i))
                  for (int v : val)
                     minArcWeight = std::min(minArcWeight, matrix[i][v]);
            out[minW] = parent.down[minW] + minArcWeight;
         }
      });

      mdd.transitionDown(d,prev,{prev},{},[minDom,prev](auto& out,const auto& parent,const auto&, const auto& val) {
         MDDBSValue sv(out[prev]);
         for(auto v : val)
            sv.set(v - minDom);
      });

      mdd.transitionDown(d,len,{len},{},[len](auto& out,const auto& in,const auto&,const auto& val) noexcept {
         out[len] = in.down[len] + 1;
      });

      mdd.addRelaxationDown(d,prev,[prev](auto& out,const auto& l,const auto& r) noexcept  {
         out[prev].setBinOR(l[prev],r[prev]);
      });

      mdd.onFixpoint([z,minW](const auto& sink) {
         z->removeBelow(sink.down[minW]);
      });
      mdd.onRestrictedFixpoint([objective,minW](const auto& sink) {
         objective->foundPrimal(sink.down[minW]);
      });

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
