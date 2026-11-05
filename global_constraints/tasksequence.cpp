// Port of Objective-CP's CPTaskSequence.
// Copyright (c) 2015 NICTA, Andreas Schutt and Pascal Van Hentenryck (MPL 2.0).
// See tasksequence.hpp for the overview.

#include <algorithm>
#include <climits>
#include "tasksequence.hpp"
#include "constraint.hpp"
#include "fail.hpp"

void TaskSequence::post()
{
   using namespace Factory;
   auto cp = _start[0]->getSolver();
   // Path 0 -> ... -> n+1 through all tasks: close it with succ[n+1] = 0 and
   // post a circuit over the n+2 nodes (Circuit also posts allDifferent).
   Veci all(_n + 2, alloci(cp->getStore()));
   for (int k = 0; k <= _n; k++) all[k] = _succ[k];
   all[_n + 1] = makeIntVar(cp, 0, 0);
   cp->post(circuit(all), false);

   for (auto& s : _succ)  s->propagateOnBind(this);
   for (auto& s : _start) s->propagateOnBoundChange(this);
   propagate();
}

void TaskSequence::propagate()
{
   int const sink = _n + 1;
   auto S   = [this](int node) -> var<int>::Ptr& { return _start[node - 1]; };
   auto D   = [this](int node) { return _dur[node - 1]; };

   // Every bound successor is a precedence (CPTaskSequence posts one on bind).
   for (int k = 1; k <= _n; k++) {
      if (!_succ[k]->isBound()) continue;
      int const v = _succ[k]->min();
      if (v == sink) continue;
      S(v)->removeBelow(S(k)->min() + D(k));
      S(k)->removeAbove(S(v)->max() - D(k));
   }

   // Walk the ranked prefix 0 -> ... -> i.
   std::fill(_inChain.begin(), _inChain.end(), 0);
   _inChain[0] = 1;
   int i = 0;
   int start = INT_MIN;     // end of the ranked prefix
   int ranked = 0;
   while (_succ[i]->isBound()) {
      int const next = _succ[i]->min();
      i = next;
      if (next == sink) break;
      if (_inChain[next]) failNow();   // cycle; Circuit also rules this out
      _inChain[next] = 1;
      ranked++;
      if (start != INT_MIN) S(next)->removeBelow(start);
      start = S(next)->min() + D(next);
   }

   // Unranked tasks start after the prefix.
   int maxLct = INT_MIN;
   int minEct = INT_MAX;
   int duration = 0;
   for (int k = 1; k <= _n; k++) {
      if (_inChain[k]) continue;
      if (start != INT_MIN) S(k)->removeBelow(start);
      maxLct = std::max(maxLct, S(k)->max() + D(k));
      minEct = std::min(minEct, S(k)->min() + D(k));
      duration += D(k);
   }
   int const unranked = _n - ranked;

   if (unranked > 0) {
      if (i == sink) failNow();            // path closed before all tasks
      _succ[i]->remove(sink);
      auto& si = _succ[i];
      int const lo = si->min(), hi = si->max();
      for (int k = lo; k <= hi; k++) {
         if (k == sink || !si->contains(k)) continue;
         // Ranking k next leaves all remaining work after est(k).
         if (S(k)->min() + duration > maxLct) {
            si->remove(k);
            S(k)->removeBelow(minEct);
         }
      }
      // The frontier task must end early enough for the remaining work.
      if (i != 0)
         S(i)->removeAbove(maxLct - duration - D(i));
   }
}
