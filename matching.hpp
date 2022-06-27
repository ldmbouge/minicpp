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

#ifndef __MATCHING_H
#define __MATCHING_H

#include <algorithm>
#include <stack>
#include <iomanip>
#include <stdint.h>
#include "intvar.hpp"

class Partition {
   int  _msz;
   int  _sz;
   int* _value;
   int* _offset;
public:
   Partition(Storage::Ptr store,int msz) : _msz(msz),_sz(0) { // [|0,1,2,3,....,msz-1] (all excluded at start)
      _value = new (store) int[msz];
      _offset = new (store) int[msz];
      for(int i=0;i<_msz;++i)
         _value[i] = _offset[i] = i;
   }
   void clear() noexcept { _sz = 0;} // this restore "in" to empty and "out" to full
   bool memberIn(int v)  const  noexcept { return _offset[v] < _sz;}
   bool memberOut(int v)  const noexcept { return _offset[v] >= _sz;}
   inline int sizeIn() const noexcept  { return _sz;}
   inline int sizeOut() const noexcept { return _msz - _sz;}
   inline const int* const in() const noexcept  { return _value;}
   inline const int* const out() const noexcept { return _value + _sz;}
    __attribute__((always_inline)) inline void include(int v) noexcept {
      assert(0 <= v && v < _msz);
      const int at = _offset[v];
      assert(_value[at]==v);
      if (at > _sz) { // somewhere in excluded. Swap and increase the size
         const int toMove = _value[_sz];
         _offset[toMove] = at;
         _value[at]      = toMove;
         _value[_sz]     = v;
         _offset[v]      = _sz++;
         return;
      } else if (at == _sz) { // at the boundary, increase the size
         ++_sz;
         return;
      } // else already in. Do nothing     
   }
    __attribute__((always_inline)) inline void exclude(int v) noexcept {
      const int at = _offset[v];
      assert(_value[at] == v);
      if (at >= _sz) // already excluded. Do nothing
         return;
      else if (at == _sz - 1) // last in segment. Just decrease size
         --_sz;
      else { // somewhere in excluded. Swap and decrease the size
         const int toMove = _value[_sz-1];
         _offset[toMove] = at;
         _value[at]      = toMove;
         _value[--_sz]   = v;
         _offset[v]      = _sz;
      }
   }
};
   
class PGraph {
   struct VTag {
      int disc;
      int low;
      bool held;
   };
   const int _nbVar,_nbVal,_V,_sink,_imin;
   int _minVal,_maxVal;
   int* _match; 
   int* _varFor;
   Factory::Veci::pointer _x;
   Partition _varsSCC,_valsSCC;
   template <typename B>
   void SCCFromVertex(const int v,B body,int& time,int u,VTag tags[],int st[],int& top,int& vCnt,bool& hasSCCSplit);
   template <typename B>
   void SCCUtil(B body,int& time,int u,VTag tags[],int st[],int& top,int& vCnt,bool& hasSCCSplit);
public:
   PGraph(Storage::Ptr store,int nbVar,int minVal,int maxVal,int match[],int varFor[],Factory::Veci::pointer x)
      : _nbVar(nbVar),
        _nbVal(maxVal - minVal + 1),
        _V(nbVar + _nbVal + 1),
        _sink(nbVar + _nbVal),
        _imin(minVal),
        _minVal(minVal),
        _maxVal(maxVal),
        _match(match),
        _varFor(varFor),
        _x(x),
        _varsSCC(store,_nbVar),
        _valsSCC(store,_nbVal)
   {
   }
   void setLiveValues(int min,int max) { _minVal = min;_maxVal = max;}
   template <typename B> void SCC(B body); // apply body to each SCC   
};

class MaximumMatching {
   Storage::Ptr _store;
   Factory::Veci& _x;
   const int _min,_max,_valSize;
   int*  _match,*_valMatch;
   int*  _varSeen,*_valSeen;
   int _szMatching;
   int _magic;
   void findInitialMatching();
   int findMaximalMatching();
   bool findAlternatingPathFromVar(int i);
   bool findAlternatingPathFromVal(int v);
public:
   MaximumMatching(Factory::Veci& x,Storage::Ptr store,int* match,int* valMatch);
   ~MaximumMatching();
   int compute();
};
   
template <typename B>
void PGraph::SCCFromVertex(const int v,B body,int& time,int u, VTag tags[],
                           int st[],int& top,int& vCnt,bool& hasSCCSplit)
{
   if (tags[v].disc == -1)  {
      SCCUtil(body,time,v,tags,st,top,vCnt,hasSCCSplit);
      tags[u].low = std::min(tags[u].low, tags[v].low);
   }
   else if (tags[v].held)
      tags[u].low = std::min(tags[u].low, tags[v].disc);
}


template <typename B>
void PGraph::SCCUtil(B body,int& time,int u, VTag tags[],int st[],int& top,int& vCnt,bool& hasSCCSplit)
{
   ++time;
   tags[u] = {time,time,true};
   st[top++] = u;
   if (u < _nbVar) { // u is a variable [0.._nbVar)
      vCnt++;
      const int xuMin = _x[u]->min(),xuMax = _x[u]->max(),mu = _match[u];
      for(int k = xuMin ; k <= xuMax ; ++k) {
         if (mu != k && _x[u]->containsBase(k)) {
            const int v = k - _imin + _nbVar;
            SCCFromVertex(v,body,time,u,tags,st,top,vCnt,hasSCCSplit); // v is a value node in the pseudo-graph
         }
      }
   } else if (u < _nbVar + _nbVal) { // u is a value. Exactly one edge out (to sink or var when matched)
      const int v = (_varFor[u - _nbVar] == -1) ? _sink : _varFor[u - _nbVar];
      SCCFromVertex(v,body,time,u,tags,st,top,vCnt,hasSCCSplit);
   } else { // u == sink: edge set going to matched values
      for(int k = _minVal; k <= _maxVal;++k) {
         if (_varFor[k - _imin] != -1) {
            const int v = _nbVar + k - _imin;  // this is the *value* node
            SCCFromVertex(v,body,time,u,tags,st,top,vCnt,hasSCCSplit);
         }
      }
   }
   if (tags[u].low == tags[u].disc)  {
      if (tags[u].low > 1 || vCnt < _nbVar)
         hasSCCSplit = true;
      if (hasSCCSplit) {
         _varsSCC.clear();
         _valsSCC.clear();
         while (st[top-1] != u) {
            const int node   = st[--top];
            if (node < _nbVar)
               _varsSCC.include(node);
            else if (node < _sink)
               _valsSCC.include(node - _nbVar);         
            tags[node].held = false;            
         }
         const int node = st[--top];
         tags[node].held = false;
         if (node < _nbVar)
            _varsSCC.include(node);
         else if (node < _sink)
            _valsSCC.include(node - _nbVar);         
         body(_varsSCC,_valsSCC);
      }
   }   
}

template <typename B> void PGraph::SCC(B body)
{
   VTag tags[_V];
   int st[_V];
   int top = 0,time = 0;
   for (int i = 0; i < _V; i++) 
      tags[i] = {-1,-1,false};

   int vCnt = 0;
   bool hasSCCSplit = false;
   for (int i = 0; i < _V; i++) {
      vCnt = 0;
      if (tags[i].disc == -1) 
         SCCUtil(body,time,i,tags,st,top,vCnt,hasSCCSplit);
   }
}

#endif
