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

#include "matching.hpp"

int minValue(Factory::Veci& x)
{
   int min = INT32_MAX;
   for(auto& xi : x) 
      min = std::min(min,xi->min());
   return min;   
}

int maxValue(Factory::Veci& x)
{
   int max = INT32_MIN;
   for(auto& xi : x) 
      max = std::max(max,xi->max());
   return max;
}

MaximumMatching::MaximumMatching(Factory::Veci& x,Storage::Ptr store,int* match,int* valMatch)
   : _store(store),_x(x),
     _min(minValue(x)),
     _max(maxValue(x)),
     _valSize(_max - _min + 1),
     _match(match),
     _valMatch(valMatch)
{
   for(int k=0;k < _valSize;k++) _valMatch[k] = -1;
   _magic = 0;
   for(auto k=0u;k < _x.size();k++) _match[k] = INT32_MIN;
   _varSeen = new (_store) int[_x.size()];
   _valSeen = new (_store) int[_valSize];
   findInitialMatching();
}

void MaximumMatching::findInitialMatching()
{
   _szMatching = 0;
   for(auto k=0u;k < _x.size();k++) {
      const int minv = _x[k]->min(),maxv = _x[k]->max();
      for(int i=minv;i <= maxv;i++) {
         if (_valMatch[i - _min] < 0)
            if (_x[k]->contains(i)) {
               _match[k] = i;
               _valMatch[i - _min] = k;
               _szMatching++;
               break;
            }
      }
   }
}

int MaximumMatching::findMaximalMatching()
{
   if (_szMatching < (int)_x.size()) {
      for(auto k=0u;k < _x.size();k++) {
         if (_match[k] == INT32_MIN) { // not matched
            _magic++;
            if (findAlternatingPathFromVar(k))
               _szMatching++;
         }
      }
   }
   return _szMatching;
}

bool MaximumMatching::findAlternatingPathFromVar(int i) {
   if (_varSeen[i] != _magic) {
      _varSeen[i] = _magic;
      const int xMin = _x[i]->min(),xMax = _x[i]->max();
      for(int v=xMin;v <= xMax;v++) {
         if (_match[i] != v) {
            if (_x[i]->contains(v)) {
               if (findAlternatingPathFromVal(v)) {
                  _match[i] = v;
                  _valMatch[v - _min] = i;
                  return true;
               }
            }
         }
      }
   }
   return false;
}

bool MaximumMatching::findAlternatingPathFromVal(int v)
{
   if (_valSeen[v - _min]  != _magic) {
      _valSeen[v - _min] = _magic;
      if (_valMatch[v - _min] == -1)
         return true;
      if (findAlternatingPathFromVar(_valMatch[v - _min]))
         return true;
   }
   return false;
}

MaximumMatching::~MaximumMatching()
{}

int MaximumMatching::compute()
{
   for(auto k=0u;k < _x.size();k++) {
      if (_match[k] != INT32_MIN) {
         if (!_x[k]->contains(_match[k])) {
            _valMatch[_match[k] - _min] = -1;
            _match[k] = INT32_MIN;
            _szMatching--;
         }
      }
   }
   return findMaximalMatching();
}
