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

// Sum constraints: s == x0 + ... + xk (bounds consistent) and sum of booleans == c.
// Factory functions are in constraint.hpp.

#pragma once

#include <vector>
#include "intvar.hpp"
#include "acstr.hpp"

class Sum : public Constraint { // s = Sum({x0,...,xk})
   Factory::Veci _x;
   trail<int>    _nUnBounds;
   trail<int>    _sumBounds;
   unsigned int _n;
   std::vector<unsigned long> _unBounds;
public:
   template <class Vec> Sum(const Vec& x,var<int>::Ptr s)
       : Constraint(s->getSolver()),
         _x(x.size() + 1,Factory::alloci(s->getStore())), 
         _nUnBounds(s->getSolver()->getStateManager(),(int)x.size()+1),
         _sumBounds(s->getSolver()->getStateManager(),0),
         _n((int)x.size() + 1),
         _unBounds(_n)
    {
       int i = 0;
       for(auto& xi : x)
          _x[i++] = xi;
       _x[_n-1] = Factory::minus(s);
       for(typename Vec::size_type i=0;i < _n;i++)
          _unBounds[i] = i;
    }
   void post() override;
   void propagate() override;
};

class SumBool : public Constraint {
   Factory::Vecb _x;
   int           _c;
   trail<int>    _nbOne,_nbZero;
public:
   template <class Vec> SumBool(const Vec& x,int c)
      : Constraint(x[0]->getSolver()),
        _x(x.size(),Factory::allocb(x[0]->getStore())),
        _c(c),
        _nbOne(x[0]->getSolver()->getStateManager(),0),
        _nbZero(x[0]->getSolver()->getStateManager(),0)
   {
       int i = 0;
       for(auto& xi : x)
          _x[i++] = xi;      
   }
   void propagateIdx(int k);
   void post() override;
};
