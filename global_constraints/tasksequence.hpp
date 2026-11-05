#pragma once

#include <vector>
#include "intvar.hpp"
#include "acstr.hpp"

// Successor model of the order of n tasks on a unary resource.
//
// Ported from Objective-CP's CPTaskSequence (Clo/Scheduler/CPScheduler),
// Copyright (c) 2015 NICTA, Andreas Schutt and Pascal Van Hentenryck (MPL 2.0).
//
// Nodes: 0 is a dummy source, 1..n are the tasks (node k is task k-1 in
// 'start'/'dur'), n+1 is a dummy sink. 'succ' has n+1 entries: succ[k] is the
// node right after node k, with values in [1, n+1]. succ[k] = v (v <= n)
// implies end(k) <= start(v).
//
// The successors form a single path 0 -> ... -> n+1 through every task
// (posted as a circuit closed by a sink->source arc). The propagator walks the
// bound prefix of that path and:
//  - pushes each ranked task after its predecessor,
//  - pushes every unranked task after the end of the ranked prefix,
//  - removes from the frontier successor any task that, if ranked next, would
//    leave the remaining work unable to finish by the largest lct,
//  - bounds the end of the frontier task so the remaining work still fits.
class TaskSequence : public Constraint {
   std::vector<var<int>::Ptr> _start;   // task k-1 is node k
   std::vector<int> const     _dur;
   std::vector<var<int>::Ptr> _succ;    // size n+1, indexed by node 0..n
   int const                  _n;
   std::vector<char>          _inChain; // scratch, indexed by node
public:
   template <typename Vars, typename Succ>
   TaskSequence(Vars& start, std::vector<int> const& dur, Succ& succ)
      : Constraint(start[0]->getSolver()),
        _start(start.begin(), start.end()), _dur(dur),
        _succ(succ.begin(), succ.end()), _n((int)start.size()),
        _inChain(_n + 2, 0)
   {
      assert(dur.size() == start.size());
      assert(succ.size() == start.size() + 1);
      setPriority(CLOW);
   }
   void post() override;
   void propagate() override;
};
