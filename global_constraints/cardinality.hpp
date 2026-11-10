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

// Cardinality (global cardinality, GCC) constraints, ported from Objective-CP
// (CPCardinality.m and CPCardinalityDC.m):
//    for every value v : low[v] <= |{ i : x[i] == v }| <= up[v]
// Bounds are given as maps from value to bound. A value missing from `low` has
// lower bound 0 and a value missing from `up` has upper bound x.size().
// Factory functions (cardinality, cardinalityDC, atLeast, atMost) are in constraint.hpp.

#pragma once

#include <map>
#include <vector>
#include "intvar.hpp"
#include "acstr.hpp"

// Counting propagator (CPCardinalityCst in Objective-CP). For each value v it counts
//   required[v] : variables bound to v
//   possible[v] : variables whose domain still contains v
// and fails when required[v] > up[v] or possible[v] < low[v]. When required[v] == up[v]
// it removes v from the unbound variables; when possible[v] == low[v] it binds every
// variable that can take v to v.
// minicpp has no per-value removal event, so the counts are recomputed on each call:
// O(n) per call when no value has a lower bound (atMost), O(sum of domain sizes) otherwise.
class Cardinality : public Constraint {
   Factory::Veci    _x;
   int              _lo,_uo;      // value range: [_lo,_uo]
   std::vector<int> _low,_up;     // bounds, indexed by v - _lo
   std::vector<int> _req,_pos;    // scratch counters, indexed by v - _lo
   bool             _hasLow;      // some value has a positive lower bound
   std::map<int,int> _lowMap,_upMap;
public:
   template <class Vec> Cardinality(const Vec& x,const std::map<int,int>& low,const std::map<int,int>& up)
      : Constraint(x[0]->getSolver()),
        _x(x.begin(),x.end(),Factory::alloci(x[0]->getStore())),
        _lowMap(low),_upMap(up) {}
   void post() override;
   void propagate() override;
};

// Domain consistent propagator (CPCardinalityDC in Objective-CP), based on flows (Regin's GCC).
// It maintains a feasible flow from the variables to the values (each value w carrying between
// low[w] and up[w] units) and removes x == w whenever the edge (x,w) is not in the flow and its
// ends belong to different strongly connected components of the residual graph.
// The flow is not trailed: it is repaired at each propagation, as in AllDifferentAC.
class CardinalityDC : public Constraint {
   static constexpr int NONE = -1;
   Factory::Veci    _x;
   int              _varSize;
   int              _valMin,_valMax,_valSize;
   std::vector<int> _low,_up;              // bounds, indexed by w - _valMin
   std::vector<int> _flow;                 // flow through each value, indexed by w - _valMin
   int              _nbAssigned;           // number of variables matched to a value
   std::vector<int> _varMatch;             // value matched to each variable (or NONE)
   std::vector<int> _valFirstMatch;        // first variable matched to a value, indexed by w - _valMin
   std::vector<int> _valNextMatch;         // next variable matched to the same value, indexed by variable
   std::vector<int> _valPrevMatch;         // previous variable matched to the same value, indexed by variable
   unsigned long              _magic;
   std::vector<unsigned long> _varMagic,_valMagic;
   int              _dfs,_component;
   std::vector<int> _varComponent,_varDfs,_varHigh;
   std::vector<int> _valComponent,_valDfs,_valHigh;   // indexed by w - _valMin
   int              _sinkComponent,_sinkDfs,_sinkHigh;
   std::vector<int> _stack,_nodeType;
   int              _top;
   std::map<int,int> _lowMap,_upMap;

   int  vi(int w) const noexcept { return w - _valMin;}
   void unmatchVar(int v);
   void matchVar(int v,int w);
   void assign(int v,int w) { unmatchVar(v); matchVar(v,w);}
   void initializeFlow();
   bool augmentVarPath(int v);
   bool augmentValPath(int w);
   bool findMaxFlow();
   bool findFeasibleFlowToValueFromVariable(int val,int v);
   bool findFeasibleFlowToValueFromValue(int val,int w);
   bool findFeasibleFlowToValue(int val);
   bool findFeasibleFlow();
   bool isFeasible();
   void SCC();
   void SCCFromVariable(int k);
   void SCCFromValue(int w);
   void SCCFromSink();
   void prune();
public:
   template <class Vec> CardinalityDC(const Vec& x,const std::map<int,int>& low,const std::map<int,int>& up)
      : Constraint(x[0]->getSolver()),
        _x(x.begin(),x.end(),Factory::alloci(x[0]->getStore())),
        _lowMap(low),_upMap(up)
   {
      setPriority(CLOW);
   }
   void post() override;
   void propagate() override;
};
