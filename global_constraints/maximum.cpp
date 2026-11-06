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

#include <string.h>
#include <limits>
#include "constraint.hpp"

void Maximum::post()
{
   for(auto& xi : _x)
      xi->propagateOnBoundChange(this);
   _y->propagateOnBoundChange(this);
   propagate();
}

void Maximum::propagate()
{
   const int yMin = _y->min(),yMax = _y->max();
   int maxOfMax = std::numeric_limits<int>::min();
   int maxOfMin = std::numeric_limits<int>::min();
   int nSupport = 0,supportIdx = -1;
   for(auto i = 0u; i < _x.size(); i++) {
      _x[i]->removeAbove(yMax);
      maxOfMax = std::max(maxOfMax,_x[i]->max());
      maxOfMin = std::max(maxOfMin,_x[i]->min());
      if (_x[i]->max() >= yMin) {    // x[i] can still be the maximum
         nSupport++;
         supportIdx = i;
      }
   }
   if (nSupport == 1)
      _x[supportIdx]->removeBelow(yMin);
   _y->removeAbove(maxOfMax);
   _y->removeBelow(maxOfMin);
}
