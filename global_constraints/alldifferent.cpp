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

void AllDifferentBinary::post()
{
   CPSolver::Ptr cp = _x[0]->getSolver();
   const long n = _x.size();
   for(int i=0;i < n;i++) 
      for(int j=i+1;j < n;j++)
         cp->post(new (cp) NEQBinBCLight(_x[i],_x[j]));    
}


void AllDifferentAC::post() 
{
   CPSolver::Ptr cp = _x[0]->getSolver();
   _nVar    = (int)_x.size();
   _nVal    = updateRange();
   _mm.setup();
   for(int i=0;i < _nVar;i++)
      _x[i]->propagateOnDomainChange(this);
   _match   = new (cp) int[_nVar];
   _varFor  = new (cp) int[_nVal];
   _nNodes  = _nVar + _nVal + 1;
   _rg = new (cp) PGraph(_nVar,_minVal,_maxVal,_match,_varFor,_x.data());
   propagate();
}

int AllDifferentAC::updateRange()
{
   _minVal = INT32_MAX;
   _maxVal = INT32_MIN;
   Factory::Veci::pointer x = _x.data();
   for(int i=0;i < _nVar;i++) {
      _minVal = std::min(_minVal,x[i]->min());
      _maxVal = std::max(_maxVal,x[i]->max());
   }
   return _maxVal - _minVal + 1;
}

void AllDifferentAC::propagate()
{
   int size = _mm.compute(_match);
   if (size < _nVar) 
      failNow();
   updateRange();
   _rg->setLiveValues(_minVal,_maxVal);
   
   for(int i=0;i < _nVal;++i) _varFor[i] = -1;
   for(int i=0;i < _nVar;++i) _varFor[_match[i] - _minVal] = i; 

   //std::cout << "DD:" << _nbd << "    " << _nbmd << "   " << _nVar << "/" << (_maxVal - _minVal + 1) << "\n";
   
   int nc = 0;
   int* scc = (int*)alloca(sizeof(int)*_nNodes);
   _rg->SCC([&scc,&nc](int n,int nd[]) {
      for(int i=0;i < n;i++)
         scc[nd[i]] = nc;
      ++nc;
   });
   if (nc > 1) {
      Factory::Veci::pointer x = _x.data();
      for(int i=0;i < _nVar;i++) {
         const int ub = x[i]->max();
         for(int v = x[i]->min(); v <= ub;++v)
            if (_match[i] != v && scc[i] != scc[valNode(v)] && x[i]->containsBase(v))
               x[i]->remove(v);
      }
   }
}
