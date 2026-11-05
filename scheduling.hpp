#ifndef __SCHEDULING_HPP
#define __SCHEDULING_HPP

#include <iostream>
#include <fstream>
#include "solver.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"

#include "ttable.hpp"
#include "global_constraints/cumulative.hpp"
#include "global_constraints/disjunctive.hpp"
#include "global_constraints/tasksequence.hpp"
#if __CUDACC__
#include "global_constraints_gpu/cumulative.cuh"
#endif

namespace Factory {
   enum CDevice { CPU, GPU };

   inline Constraint::Ptr cumulativeER(Factory::Veci &s,
                                       std::vector<int> const &p,
                                       std::vector<int> const &h, int c,
                                       enum CDevice dev = CPU) {
      switch (dev) {
        case CPU:
           return new Cumulative(s,p,h,c);
      case GPU:default:         
#ifdef __CUDACC__
          return new CumulativeGPU(s,p,h,c);
#else
          std::cerr << "Warning: this code was not compiled for NVIDIA libs. Falling back on energetic on CPU.\n";
          return new Cumulative(s, p, h, c);
#endif
     }
   }

   inline Constraint::Ptr cumulativeTT(Factory::Veci &start,
                                       const std::vector<int> &dur,
                                       const std::vector<int> &reqs, int capa,
                                       enum CDevice dev = CPU) {
     switch (dev) {
        default: // not worth running timetable on a GPU.
           return new CumulativeTT(start, dur, reqs, capa);
     }           
   }  

   // Unary resource: no two tasks with positive duration overlap.
   template <class Vars>
   inline Constraint::Ptr disjunctive(Vars &start,
                                      const std::vector<int> &dur,
                                      Disjunctive::Options opt = Disjunctive::Options()) {
      return new Disjunctive(start, dur, opt);
   }

   // Successor model of the order of the tasks on a unary resource.
   // succ has start.size()+1 entries with values in [1, start.size()+1];
   // see TaskSequence for the node numbering.
   template <class Vars, class Succ>
   inline Constraint::Ptr taskSequence(Vars &start,
                                       const std::vector<int> &dur,
                                       Succ &succ) {
      return new TaskSequence(start, dur, succ);
   }

};

// After the factories it relies on.
#include "global_constraints/disjunctiveresource.hpp"

namespace detail {
   // Ordering over the successor nodes of a resource, built from an ordering
   // over its tasks: node k (1..n) is task k-1, and the sink (n+1) compares
   // after every task, whatever the key type.
   template <class O>
   auto taskOrderToNodeOrder(int n, O& o) {
      using K = std::decay_t<decltype(o(0))>;
      return [n, &o](int node) {
         return node <= n ? std::make_pair(false, o(node - 1)) : std::make_pair(true, K{});
      };
   }
}

/**
 * One step of Objective-CP's `sequence: d.successors by: o1 then: o2` on a
 * disjunctive resource (posted), with the orderings expressed over its tasks:
 * o1(t) and o2(t) for t in [0, d->size()), the same indexing as d->est(t),
 * d->ect(t), d->start(t), ... The successor-node numbering is hidden: ending
 * the sequence is always tried after every remaining task. Returns empty
 * Branches once the resource is sequenced.
 */
template <class O1, class O2>
Branches sequence(CPSolver::Ptr cp, DisjunctiveResource::Ptr d, O1 o1, O2 o2)
{
   int const n = d->size();
   if (n == 0) return Branches({});
   return sequence(cp, d->successors(),
                   detail::taskOrderToNodeOrder(n, o1),
                   detail::taskOrderToNodeOrder(n, o2));
}

/**
 * One step of Objective-CP's `sequence: d.successors by: o` on a disjunctive
 * resource, with o expressed over its tasks (see above).
 */
template <class O1>
Branches sequence(CPSolver::Ptr cp, DisjunctiveResource::Ptr d, O1 o1)
{
   int const n = d->size();
   if (n == 0) return Branches({});
   return sequence(cp, d->successors(), detail::taskOrderToNodeOrder(n, o1));
}

#endif

