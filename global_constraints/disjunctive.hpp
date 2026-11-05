#pragma once

#include <vector>
#include "intvar.hpp"
#include "acstr.hpp"

// Unary-resource (disjunctive) constraint: no two tasks with a positive
// duration may overlap in time.  Tasks with a zero duration are ignored.
//
// Ported from Objective-CP's CPTaskDisjunctive (Clo/Scheduler/CPScheduler),
// Copyright (c) 2015 NICTA, Andreas Schutt and Pascal Van Hentenryck (MPL 2.0).
// Only the mandatory-task path is ported: minicpp has no optional or resource
// tasks, so all the *_optional_* and *MT variants are dropped.
//
// Filtering (each runs in both directions, est and lct):
//  - overload check                       (Vilim, Theta tree)        O(n log n)
//  - detectable precedences               (Vilim, Theta tree)        O(n log n)
//  - not-first / not-last                 (Vilim, Theta tree)        O(n log n)
//  - edge finding                         (Vilim, Theta-Lambda tree) O(n log n)
// Reference: P. Vilim, Global Constraints in Scheduling, PhD thesis, 2007.
struct DisjunctiveOptions {
   bool dprec = true;   // detectable precedences
   bool nfnl  = true;   // not-first / not-last
   bool ef    = true;   // edge finding
};

class Disjunctive : public Constraint {
public:
   using Options = DisjunctiveOptions;
private:
   struct ThetaNode  { int length; int time; };
   struct LambdaNode { int gLength; int gTime; };

   std::vector<var<int>::Ptr> _x;   // start times
   std::vector<int> const     _p;   // durations
   Options const              _opt;

   // Tasks with a positive duration (the only ones that matter).
   std::vector<int> _task;          // _task[k] = index into _x/_p
   int              _m;             // _task.size()

   // Snapshot of the bounds, indexed by k in [0,_m).
   std::vector<int> _est, _lst, _ect, _lct, _dur;
   std::vector<int> _newEst, _newLct;

   // Sorting permutations over [0,_m). They persist across calls so that
   // insertion sort runs on nearly sorted data (as in the original).
   std::vector<int> _byEst, _byEct, _byLst, _byLct;
   std::vector<int> _mapEst, _mapLct;   // task k -> leaf index in the tree

   int _tsize;                          // 2*_m - 1
   int _tdepth;                         // floor(log2(_tsize))
   std::vector<ThetaNode>  _theta;
   std::vector<LambdaNode> _lambda;

   void readData();
   void sortAll();
   void initIndexMap(std::vector<int> const& order, std::vector<int>& map);
   int  leafToRank(int leaf) const;
   void applyBounds();

   // Theta / Theta-Lambda tree primitives
   void initTheta(int time);
   void initLambda(int time);
   void insertThetaEct(int idx, int length, int time);
   void insertThetaLst(int idx, int length, int time);
   void insertLambdaEct(int idx, int length, int time);
   void insertLambdaLst(int idx, int length, int time);
   void initThetaLambdaEct();
   void initThetaLambdaLst();
   int  responsibleLambdaEct() const;
   int  responsibleLambdaLst() const;

   // Filtering algorithms
   void overloadCheck();
   bool dprecEst();
   bool dprecLct();
   bool nfnlEst();
   bool nfnlLct();
   bool efEst();
   bool efLct();
public:
   template <typename Vars>
   Disjunctive(Vars& x, std::vector<int> const& p, Options opt = Options())
      : Constraint(x[0]->getSolver()), _x(x.begin(), x.end()), _p(p), _opt(opt)
   {
      assert(p.size() == x.size());
      for (auto i = 0u; i < _x.size(); i++)
         if (_p[i] > 0) _task.push_back(i);
      _m = (int)_task.size();
      setPriority(CLOW);
   }
   void post() override;
   void propagate() override;
};
