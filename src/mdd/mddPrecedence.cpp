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
   MDDCstrDesc::Ptr precedenceMDD(MDD::Ptr m,const Factory::Veci& vars, int before, int after)
   {
      MDDSpec& spec = m->getSpec();
      auto desc = spec.makeConstraintDescriptor(vars,"precedenceMDD");

      const auto visitedBefore = spec.downIntState(desc,0,1,MaxFun);

      spec.arcExist(desc,[=](const auto& parent,const auto& child,const auto& x,int v) {
         if (v == after) return parent.down[visitedBefore] > 0;
         return true;
      });
      spec.transitionDown(desc,visitedBefore,{visitedBefore},{},[=](auto& out,const auto& parent,const auto&,const auto& val) {
         bool hasBefore = val.contains(before);
         out[visitedBefore] = parent.down[visitedBefore] || hasBefore;
      });
      return desc;
   }
   MDDCstrDesc::Ptr requiredPrecedenceMDD(MDD::Ptr m,const Factory::Veci& vars, int before, int after, MDDOpts opts)
   {
      MDDSpec& spec = m->getSpec();
      auto desc = spec.makeConstraintDescriptor(vars,"requiredPrecedenceMDD");

      const auto visitedBefore = spec.downIntState(desc,0,1,MaxFun,opts.cstrP);
      const auto visitedAfter = spec.upIntState(desc,0,1,MaxFun);

      spec.arcExist(desc,[before,after,visitedBefore,visitedAfter](const auto& parent,const auto& child,const auto& x,int v) {
         if (v == after && !parent.down[visitedBefore]) return false;
         if (!child.up.unused() && v == before && !child.up[visitedAfter]) return false;
         return true;
      });
      spec.transitionDown(desc,visitedBefore,{visitedBefore},{},[=](auto& out,const auto& parent,const auto&,const auto& val) {
         bool hasBefore = val.contains(before);
         out[visitedBefore] = parent.down[visitedBefore] || hasBefore;
      });
      spec.transitionUp(desc,visitedAfter,{visitedAfter},{},[=](auto& out,const auto& child,const auto&,const auto& val) {
         bool hasAfter = val.contains(after);
         out[visitedAfter] = child.up[visitedAfter] || hasAfter;
      });

      switch (opts.candP) {
         case 1:
            spec.candidateByLargest([visitedBefore](const auto& state, void* arcs, int numArcs) {
               return state[visitedBefore]*25;
            });
            break;
         default:
            break;
      }

      switch (opts.appxEQMode) {
         case 1:
            spec.equivalenceClassValue([=](const auto& down, const auto& up){
               return down[visitedBefore];
            }, opts.cstrP);
            break;
         default:
            break;
      }

      return desc;
   }
}
