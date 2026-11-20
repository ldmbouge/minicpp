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
   MDDCstrDesc::Ptr gocMDD(MDD::Ptr m,const Factory::Veci& vars, std::vector<std::pair<int,int>> requiredOrderings, MDDOpts opts)
   {
      MDDSpec& spec = m->getSpec();
      auto desc = spec.makeConstraintDescriptor(vars,"gocMDD");

      auto udom = domRange(vars);
      int minDom = udom.first;
      int domSize = udom.second - udom.first + 1;
      std::vector<std::vector<int>> priorToFollowers(domSize);
      std::vector<std::vector<int>> followerToPriors(domSize);
      bool** precedenceMatrix = (bool**)malloc(domSize * sizeof(bool*));
      for (int i = 0; i < domSize; i++) {
         precedenceMatrix[i] = (bool*)calloc(domSize, sizeof(bool));
      }
      for (auto ordering : requiredOrderings) {
	 priorToFollowers[ordering.first - minDom].push_back(ordering.second - minDom);
	 followerToPriors[ordering.second - minDom].push_back(ordering.first - minDom);
         precedenceMatrix[ordering.first][ordering.second] = true;
      }

      const auto visitedBefore  = spec.downBSState(desc,domSize,0,External,opts.cstrP);
      const auto visitedAfter  = spec.upBSState(desc,domSize,0,External,opts.cstrP);

      spec.arcExist(desc,[=](const auto& parent,const auto& child,const auto& x,int v) {
         MDDBSValue before(parent.down[visitedBefore]);
      	 for (int prior : followerToPriors[v - minDom]) {
            if (!before.getBit(prior)) return false;
         }
         if (!child.up.unused()) {
            MDDBSValue after(child.up[visitedAfter]);
      	    for (int follower : priorToFollowers[v - minDom]) {
               if (!after.getBit(follower)) return false;
            }
         }
         return true;
      });
      spec.transitionDown(desc,visitedBefore,{visitedBefore},{},[priorToFollowers,minDom,visitedBefore](auto& out,const auto& parent,const auto&,const auto& val) noexcept {
         out[visitedBefore] = parent.down[visitedBefore];
         MDDBSValue sv(out[visitedBefore]);
         for(auto v : val) {
            if (priorToFollowers[v - minDom].size())
               sv.set(v - minDom);
         }
      });
      spec.transitionUp(desc,visitedAfter,{visitedAfter},{},[followerToPriors,minDom,visitedAfter](auto& out,const auto& child,const auto&,const auto& val) noexcept {
         out[visitedAfter] = child.up[visitedAfter];
         MDDBSValue sv(out[visitedAfter]);
         for(auto v : val)
            if (followerToPriors[v - minDom].size())
               sv.set(v - minDom);
      });
      spec.addRelaxationDown(desc,visitedBefore,[visitedBefore](auto& out,const auto& l,const auto& r) noexcept  {
         out[visitedBefore].setBinOR(l[visitedBefore],r[visitedBefore]);
      });
      spec.addRelaxationUp(desc,visitedAfter,[visitedAfter](auto& out,const auto& l,const auto& r) noexcept  {
         out[visitedAfter].setBinOR(l[visitedAfter],r[visitedAfter]);
      });

      switch (opts.candP) {
         case 1:
            spec.candidateByLargest([priorToFollowers,domSize,visitedBefore](const auto& state, void* arcs, int numArcs) {
               int value = 0;
               MDDBSValue sv(state[visitedBefore]);
               for (int i = 0; i < domSize; i++) {
                  if (sv.getBit(i)) value += (priorToFollowers[i].size() * 25);
               }
               return value;
            });
            break;
         default:
            break;
      }

      switch (opts.appxEQMode) {
         case 1:
            //spec.equivalenceClassValue([=](const auto& down, const auto& up){
            //   return down[visitedBefore];
            //}, opts.cstrP);
            break;
         default:
            break;
      }

      return desc;
   }
   MDDCstrDesc::Ptr gocMDD2(MDD::Ptr m,const Factory::Veci& vars, std::vector<std::pair<int,int>> requiredOrderings, MDDOpts opts)
   {
      MDDSpec& spec = m->getSpec();
      auto desc = spec.makeConstraintDescriptor(vars,"gocMDD");

      auto udom = domRange(vars);
      int minDom = udom.first;
      int domSize = udom.second - udom.first + 1;
      std::vector<std::vector<int>> priorToFollowers(domSize);
      std::vector<std::vector<int>> followerToPriors(domSize);
      bool** precedenceMatrix = (bool**)malloc(domSize * sizeof(bool*));
      int* numPriors = (int*)calloc(domSize, sizeof(int));
      int* numFollowers = (int*)calloc(domSize, sizeof(int));
      for (int i = 0; i < domSize; i++) {
         precedenceMatrix[i] = (bool*)calloc(domSize, sizeof(bool));
      }
      for (auto ordering : requiredOrderings) {
	 priorToFollowers[ordering.first - minDom].push_back(ordering.second - minDom);
	 followerToPriors[ordering.second - minDom].push_back(ordering.first - minDom);
         precedenceMatrix[ordering.first - minDom][ordering.second - minDom] = true;
	 numPriors[ordering.second - minDom] += 1;
	 numFollowers[ordering.first - minDom] += 1;
      }

      const auto visitedBefore  = spec.downBSState(desc,domSize,0,External,opts.cstrP);
      const auto visitedAfter  = spec.upBSState(desc,domSize,0,External,opts.cstrP);
      const auto lengthDown = spec.downIntState(desc,0,domSize,MinFun, opts.cstrP);
      const auto lengthUp = spec.upIntState(desc,0,domSize,MinFun, opts.cstrP);

      spec.arcExist(desc,[=](const auto& parent,const auto& child,const auto& x,int v) {
         if (parent.down[lengthDown] < numPriors[v - minDom]) return false;
         MDDBSValue before(parent.down[visitedBefore]);
      	 for (int prior : followerToPriors[v - minDom]) {
            if (!before.getBit(prior)) return false;
         }
         if (!child.up.unused()) {
         if (child.up[lengthUp] < numFollowers[v - minDom]) return false;
            MDDBSValue after(child.up[visitedAfter]);
      	    for (int follower : priorToFollowers[v - minDom]) {
               if (!after.getBit(follower)) return false;
            }
         }
         return true;
      });
      spec.transitionDown(desc,visitedBefore,{visitedBefore},{},[numFollowers,minDom,visitedBefore](auto& out,const auto& parent,const auto&,const auto& val) noexcept {
         out[visitedBefore] = parent.down[visitedBefore];
         MDDBSValue sv(out[visitedBefore]);
         for(auto v : val) {
            if (numFollowers[v - minDom])
               sv.set(v - minDom);
         }
      });
      spec.transitionUp(desc,visitedAfter,{visitedAfter},{},[numPriors,minDom,visitedAfter](auto& out,const auto& child,const auto&,const auto& val) noexcept {
         out[visitedAfter] = child.up[visitedAfter];
         MDDBSValue sv(out[visitedAfter]);
         for(auto v : val)
            if (numPriors[v - minDom])
               sv.set(v - minDom);
      });
      spec.transitionDown(desc,lengthDown,{lengthDown},{},[lengthDown] (auto& out,const auto& parent,const auto& x,const auto& val) {
         out[lengthDown] = parent.down[lengthDown] + 1;
      });      
      spec.transitionUp(desc,lengthUp,{lengthUp},{},[lengthUp] (auto& out,const auto& child,const auto& x,const auto& val) {
         out[lengthUp] = child.up[lengthUp] + 1;
      });      
      spec.addRelaxationDown(desc,visitedBefore,[visitedBefore](auto& out,const auto& l,const auto& r) noexcept  {
         out[visitedBefore].setBinOR(l[visitedBefore],r[visitedBefore]);
      });
      spec.addRelaxationUp(desc,visitedAfter,[visitedAfter](auto& out,const auto& l,const auto& r) noexcept  {
         out[visitedAfter].setBinOR(l[visitedAfter],r[visitedAfter]);
      });

      switch (opts.candP) {
         case 1:
            spec.candidateByLargest([priorToFollowers,domSize,visitedBefore](const auto& state, void* arcs, int numArcs) {
               int value = 0;
               MDDBSValue sv(state[visitedBefore]);
               for (int i = 0; i < domSize; i++) {
                  if (sv.getBit(i)) value += (priorToFollowers[i].size() * 25);
               }
               return value;
            });
            break;
         default:
            break;
      }

      switch (opts.appxEQMode) {
         case 1:
            //spec.equivalenceClassValue([=](const auto& down, const auto& up){
            //   return down[visitedBefore];
            //}, opts.cstrP);
            break;
         default:
            break;
      }

      return desc;
   }
}
