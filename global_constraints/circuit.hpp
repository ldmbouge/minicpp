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

// Circuit constraint: the successor variables form a single Hamiltonian circuit.
// Factory functions are in constraint.hpp.

#pragma once

#include <vector>
#include "intvar.hpp"
#include "acstr.hpp"

class Circuit :public Constraint {
   Factory::Veci  _x;
   trail<int>* _dest;
   trail<int>* _orig;
   trail<int>* _lengthToDest;
   void bind(int i);
   void setup(CPSolver::Ptr cp);
public:
   template <class Vec> Circuit(const Vec& x)
      : Constraint(x.front()->getSolver()),
        _x(x.size(),Factory::alloci(x.front()->getStore()))
   {
      int i = 0;
      auto cp = x[0]->getSolver();
      for(auto& xi : x)
         _x[i++] = xi;
      setup(cp);
   }
   void post() override;
};
