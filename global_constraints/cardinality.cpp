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

// Ported from Objective-CP: CPCardinality.m (Cardinality) and CPCardinalityDC.m (CardinalityDC).

#include <algorithm>
#include <climits>
#include "constraint.hpp"

// Computes the value range [lo,uo] (the union of the domains and the values named in the
// bounds) and the dense bound vectors indexed by v - lo. A value missing from `low` gets 0,
// a value missing from `up` gets x.size(). Fails when some low[v] > up[v].
static void cardinalityBounds(const Factory::Veci& x,const std::map<int,int>& low,const std::map<int,int>& up,
                              int& lo,int& uo,std::vector<int>& lb,std::vector<int>& ub)
{
   const int n = (int)x.size();
   lo = INT_MAX;
   uo = INT_MIN;
   for(const auto& xi : x) {
      lo = std::min(lo,xi->min());
      uo = std::max(uo,xi->max());
   }
   for(const auto& [v,b] : low) { lo = std::min(lo,v); uo = std::max(uo,v);}
   for(const auto& [v,b] : up)  { lo = std::min(lo,v); uo = std::max(uo,v);}
   const int sz = uo - lo + 1;
   lb.assign(sz,0);
   ub.assign(sz,n);
   for(const auto& [v,b] : low) lb[v - lo] = std::max(b,0);
   for(const auto& [v,b] : up)  ub[v - lo] = std::min(b,n);
   for(int k = 0;k < sz;k++)
      if (lb[k] > ub[k])
         failNow();
}

// ---------------------------------------------------------------------------------------
// Cardinality (counting propagator)

void Cardinality::post()
{
   cardinalityBounds(_x,_lowMap,_upMap,_lo,_uo,_low,_up);
   _hasLow = std::any_of(_low.begin(),_low.end(),[](int b) { return b > 0;});
   _req.resize(_low.size());
   _pos.resize(_low.size());
   for(auto& xi : _x) {
      if (xi->isBound()) continue;
      if (_hasLow)
         xi->propagateOnDomainChange(this);  // possible[v] changes when any value is lost
      else
         xi->propagateOnBind(this);          // only required[v] matters
   }
   propagate();
}

void Cardinality::propagate()
{
   const int sz = (int)_low.size();
   std::fill(_req.begin(),_req.end(),0);
   std::fill(_pos.begin(),_pos.end(),0);
   for(const auto& xi : _x) {
      if (xi->isBound()) {
         const int k = xi->min() - _lo;
         _req[k]++;
         _pos[k]++;
      } else if (_hasLow) {
         for(int v = xi->min();v <= xi->max();v++)
            _pos[v - _lo] += xi->contains(v);
      }
   }
   for(int k = 0;k < sz;k++)
      if (_req[k] > _up[k] || (_hasLow && _pos[k] < _low[k]))
         failNow();
   // Counts only become more constraining as the rules fire, so acting on this snapshot is
   // sound; the domain changes reschedule the constraint, which then sees the new counts.
   for(int k = 0;k < sz;k++) {
      const int v = k + _lo;
      if (_req[k] == _up[k]) {                                   // v is full: nobody else takes it
         for(auto& xi : _x)
            if (!xi->isBound() && xi->contains(v))
               xi->remove(v);
      } else if (_low[k] > 0 && _pos[k] == _low[k]) {            // v needs every candidate
         for(auto& xi : _x)
            if (!xi->isBound() && xi->contains(v))
               xi->assign(v);
      }
   }
}

// ---------------------------------------------------------------------------------------
// CardinalityDC (flow based, domain consistent)

void CardinalityDC::post()
{
   _varSize = (int)_x.size();
   cardinalityBounds(_x,_lowMap,_upMap,_valMin,_valMax,_low,_up);
   _valSize = _valMax - _valMin + 1;
   _varComponent.assign(_varSize,0);
   _varDfs.assign(_varSize,0);
   _varHigh.assign(_varSize,0);
   _valComponent.assign(_valSize,0);
   _valDfs.assign(_valSize,0);
   _valHigh.assign(_valSize,0);
   _stack.assign(_varSize + _valSize + 1,0);
   _nodeType.assign(_varSize + _valSize + 1,0);
   initializeFlow();
   propagate();
   for(auto& xi : _x)
      if (!xi->isBound())
         xi->propagateOnDomainChange(this);
}

void CardinalityDC::unmatchVar(int v)
{
   if (_varMatch[v] == NONE)
      return;
   const int w = _varMatch[v];
   _nbAssigned--;
   _flow[vi(w)]--;
   const int p = _valPrevMatch[v];
   const int n = _valNextMatch[v];
   _varMatch[v] = NONE;
   _valNextMatch[v] = NONE;
   _valPrevMatch[v] = NONE;
   if (p != NONE)
      _valNextMatch[p] = n;
   else
      _valFirstMatch[vi(w)] = n;
   if (n != NONE)
      _valPrevMatch[n] = p;
}

void CardinalityDC::matchVar(int v,int w)
{
   _nbAssigned++;
   _flow[vi(w)]++;
   _varMatch[v] = w;
   const int cv = _valFirstMatch[vi(w)];
   _valFirstMatch[vi(w)] = v;
   _valNextMatch[v] = cv;
   _valPrevMatch[v] = NONE;
   if (cv != NONE)
      _valPrevMatch[cv] = v;
}

void CardinalityDC::initializeFlow()
{
   _magic = 0;
   _varMatch.assign(_varSize,NONE);
   _valNextMatch.assign(_varSize,NONE);
   _valPrevMatch.assign(_varSize,NONE);
   _varMagic.assign(_varSize,0);
   _flow.assign(_valSize,0);
   _valFirstMatch.assign(_valSize,NONE);
   _valMagic.assign(_valSize,0);
   _nbAssigned = 0;
   for(int i = 0;i < _varSize;i++) {               // greedy initial flow
      const auto& x = _x[i];
      for(int v = x->min();v <= x->max();v++)
         if (_flow[vi(v)] < _up[vi(v)] && x->contains(v)) {
            matchVar(i,v);
            break;
         }
   }
}

bool CardinalityDC::augmentVarPath(int i)
{
   if (_varMagic[i] != _magic) {
      _varMagic[i] = _magic;
      const auto& x = _x[i];
      for(int w = x->min();w <= x->max();w++)
         if (_varMatch[i] != w && x->contains(w))
            if (augmentValPath(w)) {
               assign(i,w);
               return true;
            }
   }
   return false;
}

bool CardinalityDC::augmentValPath(int w)
{
   const int k = vi(w);
   if (_valMagic[k] != _magic) {
      _valMagic[k] = _magic;
      if (_flow[k] < _up[k])        // forward
         return true;
      if (_flow[k] == 0)            // cannot borrow
         return false;
      for(int v = _valFirstMatch[k];v != NONE;v = _valNextMatch[v])
         if (augmentVarPath(v))
            return true;
   }
   return false;
}

bool CardinalityDC::findMaxFlow()
{
   if (_nbAssigned < _varSize) {
      for(int v = 0;v < _varSize;v++)
         if (_varMatch[v] == NONE) {
            _magic++;
            if (!augmentVarPath(v))
               return false;
         }
   }
   return true;
}

bool CardinalityDC::findFeasibleFlowToValueFromVariable(int val,int i)
{
   if (_varMagic[i] != _magic) {   // forward
      _varMagic[i] = _magic;
      const auto& x = _x[i];
      if (_varMatch[i] != val && x->contains(val)) {
         assign(i,val);
         return true;
      }
      for(int w = x->min();w <= x->max();w++)
         if (w != val && _varMatch[i] != w && x->contains(w))
            if (findFeasibleFlowToValueFromValue(val,w)) {
               assign(i,w);
               return true;
            }
   }
   return false;
}

bool CardinalityDC::findFeasibleFlowToValueFromValue(int val,int w)
{
   const int k = vi(w);
   if (_valMagic[k] != _magic) {
      _valMagic[k] = _magic;
      for(int v = _valFirstMatch[k];v != NONE;v = _valNextMatch[v])
         if (findFeasibleFlowToValueFromVariable(val,v))
            return true;
   }
   return false;
}

bool CardinalityDC::findFeasibleFlowToValue(int val)
{
   for(int w = _valMin;w <= _valMax;w++)   // borrowing
      if (_flow[vi(w)] > _low[vi(w)])
         if (findFeasibleFlowToValueFromValue(val,w))
            return true;
   return false;
}

bool CardinalityDC::findFeasibleFlow()
{
   for(int w = _valMin;w <= _valMax;w++)
      while (_flow[vi(w)] < _low[vi(w)]) {
         _magic++;
         if (!findFeasibleFlowToValue(w))
            return false;
      }
   return true;
}

bool CardinalityDC::isFeasible()
{
   for(int i = 0;i < _varSize;i++)    // drop the flow on edges whose value left the domain
      if (_varMatch[i] != NONE && !_x[i]->contains(_varMatch[i]))
         unmatchVar(i);
   return findMaxFlow() && findFeasibleFlow();
}

// Residual graph: variable -> value for edges not in the flow, value -> variable for flow edges,
// value -> sink when flow[w] < up[w], sink -> value when flow[w] > low[w].
void CardinalityDC::SCC()
{
   std::fill(_varComponent.begin(),_varComponent.end(),0);
   std::fill(_varDfs.begin(),_varDfs.end(),0);
   std::fill(_valComponent.begin(),_valComponent.end(),0);
   std::fill(_valDfs.begin(),_valDfs.end(),0);
   _sinkComponent = 0;
   _sinkDfs = 0;
   _top = 0;
   _dfs = _varSize + _valSize + 1;
   _component = 0;
   for(int v = 0;v < _varSize;v++)
      if (!_varDfs[v])
         SCCFromVariable(v);
}

void CardinalityDC::SCCFromVariable(int k)
{
   _varDfs[k] = _dfs--;
   _varHigh[k] = _varDfs[k];
   _stack[_top] = k;
   _nodeType[_top] = 0;
   _top++;
   const auto& x = _x[k];
   for(int w = x->min();w <= x->max();w++) {
      if (_varMatch[k] != w && x->contains(w)) {
         const int j = vi(w);
         if (!_valDfs[j]) {
            SCCFromValue(w);
            if (_valHigh[j] > _varHigh[k])
               _varHigh[k] = _valHigh[j];
         } else if (_valDfs[j] > _varDfs[k] && !_valComponent[j]) {
            if (_valDfs[j] > _varHigh[k])
               _varHigh[k] = _valDfs[j];
         }
      }
   }
   if (_varHigh[k] == _varDfs[k]) {
      _component++;
      while (true) {
         _top--;
         const int y = _stack[_top];
         const int t = _nodeType[_top];
         if (t == 0) {
            _varComponent[y] = _component;
            if (y == k)
               break;
         } else if (t == 1)
            _valComponent[vi(y)] = _component;
         else
            _sinkComponent = _component;
      }
   }
}

void CardinalityDC::SCCFromValue(int w)
{
   const int j = vi(w);
   _valDfs[j] = _dfs--;
   _valHigh[j] = _valDfs[j];
   _stack[_top] = w;
   _nodeType[_top] = 1;
   _top++;
   for(int v = _valFirstMatch[j];v != NONE;v = _valNextMatch[v]) {
      if (!_varDfs[v]) {
         SCCFromVariable(v);
         if (_varHigh[v] > _valHigh[j])
            _valHigh[j] = _varHigh[v];
      } else if (_varDfs[v] > _valDfs[j] && !_varComponent[v]) {
         if (_varDfs[v] > _valHigh[j])
            _valHigh[j] = _varDfs[v];
      }
   }
   if (_flow[j] < _up[j]) {
      if (!_sinkDfs) {
         SCCFromSink();
         if (_sinkHigh > _valHigh[j])
            _valHigh[j] = _sinkHigh;
      } else if (_sinkDfs > _valDfs[j] && !_sinkComponent) {
         if (_sinkDfs > _valHigh[j])
            _valHigh[j] = _sinkDfs;
      }
   }
   if (_valHigh[j] == _valDfs[j]) {
      _component++;
      while (true) {
         _top--;
         const int y = _stack[_top];
         const int t = _nodeType[_top];
         if (t == 0)
            _varComponent[y] = _component;
         else if (t == 1) {
            _valComponent[vi(y)] = _component;
            if (y == w)
               break;
         } else
            _sinkComponent = _component;
      }
   }
}

void CardinalityDC::SCCFromSink()
{
   _sinkDfs  = _dfs--;
   _sinkHigh = _sinkDfs;
   _stack[_top] = 0;
   _nodeType[_top] = 2;
   _top++;
   for(int w = _valMin;w <= _valMax;w++) {
      const int j = vi(w);
      if (_flow[j] > _low[j]) {
         if (!_valDfs[j]) {
            SCCFromValue(w);
            if (_valHigh[j] > _sinkHigh)
               _sinkHigh = _valHigh[j];
         } else if (_valDfs[j] > _sinkDfs && !_valComponent[j]) {
            if (_valDfs[j] > _sinkHigh)
               _sinkHigh = _valDfs[j];
         }
      }
   }
   if (_sinkHigh == _sinkDfs) {
      _component++;
      while (true) {
         _top--;
         const int y = _stack[_top];
         const int t = _nodeType[_top];
         if (t == 0)
            _varComponent[y] = _component;
         else if (t == 1)
            _valComponent[vi(y)] = _component;
         else {
            _sinkComponent = _component;
            break;
         }
      }
   }
}

void CardinalityDC::prune()
{
   SCC();
   for(int v = 0;v < _varSize;v++) {
      const auto& x = _x[v];
      for(int w = x->min();w <= x->max();w++)
         if (_varMatch[v] != w && _varComponent[v] != _valComponent[vi(w)] && x->contains(w))
            x->remove(w);
   }
}

void CardinalityDC::propagate()
{
   if (!isFeasible())
      failNow();
   prune();
}
