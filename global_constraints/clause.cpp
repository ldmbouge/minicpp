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

Clause::Clause(const std::vector<var<bool>::Ptr>& x)
   : Constraint(x[0]->getSolver()),
     _wL(x[0]->getSolver()->getStateManager(),0),
     _wR(x[0]->getSolver()->getStateManager(),(int)x.size() - 1)
{
   for(auto xi : x) _x.push_back(xi);
}

void Clause::propagate()
{
   const long n = _x.size();
   int i = _wL;
   while (i < n && _x[i]->isBound()) {
      if (_x[i]->isTrue()) {
         setActive(false);
         return;
      }
      i += 1;
   }
   _wL = i;
   i = _wR;
   while (i>=0 && _x[i]->isBound()) {
      if (_x[i]->isTrue()) {
         setActive(false);
         return;
      }
      i -= 1;
   }
   _wR = i;
   if (_wL > _wR) failNow();
   else if (_wL == _wR) {
      _x[_wL]->assign(true);
      setActive(false);
   } else {
      assert(_wL != _wR);
      assert(!_x[_wL]->isBound());
      assert(!_x[_wR]->isBound());
      _x[_wL]->propagateOnBind(this);
      _x[_wR]->propagateOnBind(this);
   }
}

IsClause::IsClause(var<bool>::Ptr b,const std::vector<var<bool>::Ptr>& x)
   : Constraint(x[0]->getSolver()),
     _b(b),
     _unBounds(x.size()),
     _nUnBounds(x[0]->getSolver()->getStateManager(),(int)x.size())
{
   for(auto xi : x) _x.push_back(xi);
   for(auto i = 0u; i < _x.size();i++)
      _unBounds[i] = i;
   _clause = new (x[0]->getSolver()) Clause(x);
}

void IsClause::post()
{
   _b->propagateOnBind(this);
   for(auto& xi : _x)
      xi->propagateOnBind(this);
}

void IsClause::propagate()
{
   auto cp = _x[0]->getSolver();
   if (_b->isTrue()) {
      setActive(false);
      cp->post(_clause,false);
   } else if (_b->isFalse()) {
      for(auto& xi : _x)
         xi->assign(false);
      setActive(false);
   } else {
      int nU = _nUnBounds;
      for(int i = nU - 1;i >=0;i--) {
         int idx = _unBounds[i];
         auto y = _x[idx];
         if (y->isBound()) {
            if (y->isTrue()) {
               _b->assign(true);
               setActive(false);
               return;
            }
            _unBounds[i] = _unBounds[nU -1];
            _unBounds[nU - 1] = idx;
            nU--;
         }
      }
      if (nU == 0) {
         _b->assign(false);
         setActive(false);
      }
      _nUnBounds = nU;
   }
}
