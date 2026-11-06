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

void Sum::post()
{
   for(auto& var : _x)
      var->propagateOnBoundChange(this);
   propagate();
}

void Sum::propagate()
{  
   int nU = _nUnBounds;
   int sumMin = _sumBounds,sumMax = _sumBounds;
   var<int>::Ptr* x = _x.data();
   for(int i = nU - 1; i >= 0;i--) {
      const auto idx = _unBounds[i];
      const int vMin = x[idx]->min();
      const int vMax = x[idx]->max();
      sumMin += vMin;
      sumMax += vMax;
      if (vMin == vMax) {
         _sumBounds = _sumBounds + vMin;
         _unBounds[i] = _unBounds[nU - 1];
         _unBounds[nU - 1] = idx;
         nU--;
      }
   }
   //_nUnBounds = nU;
   if (0 < sumMin ||  sumMax < 0)
      failNow();
   for(int i = nU - 1; i >= 0;i--) {
      auto idx = _unBounds[i];
      const auto xU = -(sumMin - x[idx]->min()),xL = -(sumMax - x[idx]->max());
      x[idx]->removeAbove(xU);
      x[idx]->removeBelow(xL);
      if (xL == xU) {
         _sumBounds += xL;
         _unBounds[i] = _unBounds[--nU];
         _unBounds[nU] = idx;
      }
   }
   _nUnBounds = nU;
   if (nU == 0) setActive(false);
}

void SumBool::post() 
{
   int nbTrue = 0,nbPos = 0;
   for(auto i=0u;i < _x.size();i++) {
      nbTrue += _x[i]->min() == 1;
      nbPos  += !_x[i]->isBound();
   }
   if (nbTrue > _c)
      failNow();
   if (nbTrue == _c) {
      for(auto xi : _x)
         if (!xi->isBound())
            xi->assign(false);
      return ;
   }
   if (nbTrue + nbPos < _c)
      failNow();
   if (nbTrue + nbPos == _c) {
      for(auto xi : _x)
         if (!xi->isBound())
            xi->assign(true);
      return ;
   }
   _nbOne = nbTrue;
   _nbZero = (int)_x.size() - nbTrue - nbPos;
   for(auto k=0u;k < _x.size();k++) {
      if (_x[k]->isBound()) continue;
      _x[k]->whenBind([this,k]() {
         this->propagateIdx(k);
      });
   }
}

void SumBool::propagateIdx(int k)
{
   int nb1 = 0;
   if (_x[k]->isTrue()) {
      if (_nbOne + 1 == _c) {
         for(auto i=0u;i < _x.size();i++) {
            nb1 += _x[i]->isTrue();
            if (!_x[i]->isBound())
               _x[i]->assign(false);
         }
         if (nb1 != _c)
            failNow();
      } else
         _nbOne = _nbOne + 1;
   } else {
      if ((int)_x.size() - _nbZero - 1 == _c) {
         for(auto i=0u;i < _x.size();i++) {
            nb1 += _x[i]->min();
            if (!_x[i]->isBound()) {
               _x[i]->assign(true);
               ++nb1; 
            }
         }
         if (nb1 != _c)
            failNow();
      } else
         _nbZero = _nbZero + 1;
   }
}
