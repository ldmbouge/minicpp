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

// AllDifferent constraints: binary decomposition and arc consistent (matching based).
// Factory functions are in constraint.hpp.

#pragma once

#include <vector>
#include "intvar.hpp"
#include "acstr.hpp"
#include "matching.hpp"

class AllDifferentBinary :public Constraint {
   Factory::Veci _x;
public:
  AllDifferentBinary(const Factory::Veci& x)
    : Constraint(x[0]->getSolver()),
      _x(x.size(),Factory::alloci(x[0]->getStore())) {
    int i  = 0;
    for(auto& xi : x)
      _x[i++] = xi;
  }
  template <class Vec> AllDifferentBinary(const Vec& x)
    : Constraint(x[0]->getSolver()),
      _x(x.size(),Factory::alloci(x[0]->getStore()))
   {
     int i  = 0;
     for(auto& xi : x)
       _x[i++] = xi;
   }
  void post() override;
};

class AllDifferentAC : public Constraint {
   Factory::Veci    _x;
   MaximumMatching _mm;
   PGraph*         _rg;
   int* _match,*_varFor;
   int _minVal,_maxVal;
   int _nVar,_nVal,_nNodes;
   int updateRange();
   int valNode(int vid) const noexcept { return vid - _minVal + _nVar;}
public:
   template <class Vec> AllDifferentAC(const Vec& x)
      : Constraint(x[0]->getSolver()),
        _x(x.begin(),x.end(),Factory::alloci(x[0]->getStore())),
        _mm(_x,x[0]->getStore()) {}
   ~AllDifferentAC() {}
   void post() override;
   void propagate() override;
};
