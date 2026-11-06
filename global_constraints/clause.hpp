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

// Clause constraints: x0 OR ... OR xn, and its reified form b <=> x0 OR ... OR xn.
// Factory functions are in constraint.hpp.

#pragma once

#include <vector>
#include "intvar.hpp"
#include "acstr.hpp"

class Clause : public Constraint { // x0 OR x1 .... OR xn
   std::vector<var<bool>::Ptr> _x;
   trail<int> _wL,_wR;
public:
   Clause(const std::vector<var<bool>::Ptr>& x);
   void post() override { propagate();}
   void propagate() override;
};

class IsClause : public Constraint { // b <=> x0 OR .... OR xn
   var<bool>::Ptr _b;
   std::vector<var<bool>::Ptr> _x;
   std::vector<int> _unBounds;
   trail<int>      _nUnBounds;
   Clause::Ptr        _clause;
public:
   IsClause(var<bool>::Ptr b,const std::vector<var<bool>::Ptr>& x);
   void post() override;
   void propagate() override;
};
