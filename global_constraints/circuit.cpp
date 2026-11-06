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

void Circuit::setup(CPSolver::Ptr cp)
{
   _dest = new (cp) trail<int>[_x.size()];
   _orig = new (cp) trail<int>[_x.size()];
   _lengthToDest = new (cp) trail<int>[_x.size()];
   for(auto i=0u;i<_x.size();i++) {
      new (_dest+i) trail<int>(cp->getStateManager(),i);
      new (_orig+i) trail<int>(cp->getStateManager(),i);
      new (_lengthToDest+i) trail<int>(cp->getStateManager(),0);
   }
}

void Circuit::post()
{
   auto cp = _x[0]->getSolver();
   cp->post(Factory::allDifferent(_x));
   if (_x.size() == 1) {
      _x[0]->assign(0);
      return ;      
   }
   for(auto i=0u;i < _x.size();i++)
      _x[i]->remove(i);
   for(auto i=0u;i < _x.size();i++) {
      if (_x[i]->isBound())
         bind(i);
      else 
         _x[i]->whenBind([i,this]() { bind(i);});
   }
}

void Circuit::bind(int i)
{
   int j = _x[i]->min();
   int origi = _orig[i];
   int destj = _dest[j];
   _dest[origi] = destj;
   _orig[destj] = origi;
   int length = _lengthToDest[origi] + _lengthToDest[j] + 1;
   _lengthToDest[origi] = length;
   if (length < (int)_x.size() - 1)
      _x[destj]->remove(origi);
}
