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

// Maximum constraint: y == max(x0, ..., xk).
// Factory functions are in constraint.hpp.

#pragma once

#include <vector>
#include "intvar.hpp"
#include "acstr.hpp"

class Maximum : public Constraint { // y == max(x0,...,xk)
   Factory::Veci _x;
   var<int>::Ptr _y;
public:
   template <class Vec> Maximum(const Vec& x,var<int>::Ptr y)
      : Constraint(y->getSolver()),
        _x(x.size(),Factory::alloci(y->getStore())),
        _y(y)
   {
      int i = 0;
      for(auto& xi : x)
         _x[i++] = xi;
   }
   void post() override;
   void propagate() override;
};
