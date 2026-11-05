#pragma once

#include <vector>
#include "intvar.hpp"
#include "acstr.hpp"

// A unary (disjunctive) resource, modelled after Objective-CP's
// ORTaskDisjunctive: tasks are first associated with the resource through
// add(), then posting the resource imposes the disjunctive requirements on
// exactly those tasks:
//   - a Disjunctive constraint (no two tasks overlap), and
//   - a TaskSequence over fresh successor variables (see successors()).
//
//   auto machines = Factory::disjunctiveArray(cp, nbMachines);
//   machines[m]->add(start, duration);   // for every task on machine m
//   for (auto& d : machines) cp->post(d);
//
// The resource also offers the slack measures of Objective-CP's
// CPTaskDisjunctive, used to order resources during search.
class DisjunctiveResource : public Constraint {
   CPSolver::Ptr              _cp;
   std::vector<var<int>::Ptr> _start;
   std::vector<int>           _dur;
   std::vector<var<int>::Ptr> _succ;   // empty until posted
   bool                       _posted;
public:
   typedef handle_ptr<DisjunctiveResource> Ptr;
   DisjunctiveResource(CPSolver::Ptr cp)
      : Constraint(cp), _cp(cp), _posted(false) {}

   // Associates a task with this resource. Only allowed before posting.
   void add(var<int>::Ptr start, int duration);
   // Creates the successor variables and posts Disjunctive + TaskSequence.
   void post() override;
   void propagate() override {}

   int  size() const                                    { return (int)_start.size(); }
   var<int>::Ptr start(int k) const                     { return _start[k]; }
   int  duration(int k) const                           { return _dur[k]; }
   int  est(int k) const                                { return _start[k]->min(); }
   int  ect(int k) const                                { return _start[k]->min() + _dur[k]; }
   int  lst(int k) const                                { return _start[k]->max(); }
   int  lct(int k) const                                { return _start[k]->max() + _dur[k]; }
   std::vector<var<int>::Ptr> const& starts() const     { return _start; }
   std::vector<int> const&           durations() const  { return _dur; }

   // Successor model (valid once posted): nodes 0 (source), 1..size() (task
   // k-1 is node k) and size()+1 (sink); successors()[k] is the node after k.
   std::vector<var<int>::Ptr> const& successors() const { return _succ; }
   // True when the successor path from the source reaches the sink.
   bool isSequenced() const;
   // First node on the path from the source whose successor is unbound,
   // or -1 when the resource is sequenced.
   int  frontier() const;

   // Slack measures from CPTaskDisjunctive (getGlobalSlack / getLocalSlack).
   // Tasks with zero duration are ignored; only unfixed tasks bound the windows.
   int globalSlack() const;
   int localSlack() const;
};

namespace Factory {
   inline DisjunctiveResource::Ptr disjunctiveResource(CPSolver::Ptr cp) {
      return new (cp) DisjunctiveResource(cp);
   }
   inline std::vector<DisjunctiveResource::Ptr> disjunctiveArray(CPSolver::Ptr cp, int n) {
      std::vector<DisjunctiveResource::Ptr> a;
      for (int i = 0; i < n; i++) a.push_back(disjunctiveResource(cp));
      return a;
   }
}
