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
      for (auto ordering : requiredOrderings) {
	 priorToFollowers[ordering.first - minDom].push_back(ordering.second - minDom);
	 followerToPriors[ordering.second - minDom].push_back(ordering.first - minDom);
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
      spec.transitionDown(desc,visitedBefore,{visitedBefore},{},[minDom,visitedBefore](auto& out,const auto& parent,const auto&,const auto& val) noexcept {
         out[visitedBefore] = parent.down[visitedBefore];
         MDDBSValue sv(out[visitedBefore]);
         for(auto v : val)
            sv.set(v - minDom);
      });
      spec.transitionUp(desc,visitedAfter,{visitedAfter},{},[minDom,visitedAfter](auto& out,const auto& child,const auto&,const auto& val) noexcept {
         out[visitedAfter] = child.up[visitedAfter];
         MDDBSValue sv(out[visitedAfter]);
         for(auto v : val)
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
}
