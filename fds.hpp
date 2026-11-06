#pragma once
// Failure-Directed Search (FDS) for scheduling: port of MaxiCP's
// org.maxicp.search.FDS (MIT License, (c) 2024 UCLouvain, Pierre Schaus).
//
// Reference: P. Vilim, P. Laborie, P. Shaw. Failure-directed Search for
// Constraint-based Scheduling. CPAIOR 2015.
//
// FDS assumes that there is no (better) solution and heads into conflicts:
// it branches on binary choices "start <= t" / "start > t", picks the choice
// whose branches failed most recently (lowest rating), and explores first the
// branch most likely to fail.
//
// Differences with MaxiCP: minicpp has no interval variables, so a task is a
// start variable (its duration is fixed), and there are only start-time
// choices (no presence or length choices). Like MaxiCP's FDS, there are no
// restarts, nogoods or strong branching.
//
// MaxiCP's choices for the details the paper leaves open are kept:
//  - initial choices: binary splitting of each start domain at its midpoints,
//    breadth first, at most 30 per variable; when every choice is resolved but
//    a start is not fixed, one more midpoint choice per unfixed start is added
//    (choices are never removed);
//  - the best choice is the unresolved one with the lowest rating (ties: the
//    first created);
//  - localRating = 0 if the branch fails immediately, 1 + R otherwise, where R
//    is the product over all starts of (domain size after / before), at most 1;
//  - rating = alpha * rating + (1 - alpha) * localRating / avgRating[depth],
//    where avgRating[depth] is the running mean of localRating at that depth,
//    the depth being counted from where the search started.
//
// Usage: DFSearch search(cp, land(fds(cp, starts), ...));
// The branching returns no branch once every start is fixed.

#include <algorithm>
#include <climits>
#include <cmath>
#include <deque>
#include <functional>
#include <memory>
#include <utility>
#include <vector>
#include "fail.hpp"
#include "intvar.hpp"
#include "search.hpp"
#include "solver.hpp"

class FDS {
   struct Choice {
      int var;              // index in _x
      int split;            // positive: start <= split, negative: start > split
      double pos = 1.0;     // rating of the positive branch
      double neg = 1.0;     // rating of the negative branch
      double rating() const { return pos + neg; }
   };
   static constexpr int MaxInitialChoicesPerVar = 30;

   CPSolver::Ptr _cp;
   std::vector<var<int>::Ptr> _x;
   double _alpha;
   int _baseDepth;
   std::vector<Choice> _choices;
   std::vector<std::pair<double, double>> _avg;   // per depth: (running mean, count)
   std::vector<long long> _sizeBefore;            // domain sizes before a branch

   static long long size(var<int>::Ptr const& x) {
      return std::max<long long>(1, (long long)x->max() - x->min() + 1);
   }
   bool resolved(Choice const& c) const {
      auto const& x = _x[c.var];
      return x->max() <= c.split || x->min() > c.split;
   }
   bool allFixed() const {
      for (auto const& x : _x)
         if (!x->isBound()) return false;
      return true;
   }
   void addSplits(int var, int lo, int hi) {   // midpoints, breadth first
      std::deque<std::pair<int, int>> queue{{lo, hi}};
      int count = 0;
      while (!queue.empty() && count < MaxInitialChoicesPerVar) {
         auto [l, h] = queue.front();
         queue.pop_front();
         if (l >= h) continue;
         int const mid = l + (h - l) / 2;
         _choices.push_back({var, mid});
         count++;
         queue.push_back({l, mid});
         queue.push_back({mid + 1, h});
      }
   }
   void addMoreChoices() {
      for (int i = 0; i < (int)_x.size(); i++)
         if (!_x[i]->isBound())
            _choices.push_back({i, _x[i]->min() + (_x[i]->max() - _x[i]->min()) / 2});
   }
   int depth() const { return std::max(0, _cp->getStateManager()->depth() - _baseDepth); }

   void snapshot() {
      for (size_t i = 0; i < _x.size(); i++) _sizeBefore[i] = size(_x[i]);
   }
   double reduction() const {   // R: product of the size ratios, as a sum of logs
      double logR = 0.0;
      for (size_t i = 0; i < _x.size(); i++)
         logR += std::log((double)size(_x[i]) / (double)_sizeBefore[i]);
      return std::min(1.0, std::exp(logR));
   }
   void updateRating(int choice, bool positive, bool failed, double R) {
      double const local = failed ? 0.0 : 1.0 + R;
      int const d = depth();
      if ((int)_avg.size() <= d) _avg.resize(d + 1, {1.0, 0.0});
      auto& [mean, count] = _avg[d];
      double const normalized = local / (mean > 0 ? mean : 1.0);
      Choice& c = _choices[choice];
      double& r = positive ? c.pos : c.neg;
      r = _alpha * r + (1.0 - _alpha) * normalized;
      count += 1.0;
      mean += (local - mean) / count;
   }
   // One branch of a choice: apply it, propagate, update its rating. A failure
   // is caught to rate the branch, then propagated as usual. No heap memory is
   // allocated here (failures are longjmps by default).
   std::function<void(void)> branch(int choice, bool positive) {
      return [this, choice, positive]() {
         Choice const c = _choices[choice];
         snapshot();
         TRYFAIL
            if (positive)
               _x[c.var]->removeAbove(c.split);
            else
               _x[c.var]->removeBelow(c.split + 1);
            _cp->fixpoint();
         ONFAIL
            updateRating(choice, positive, true, 0.0);
            failNow();
         ENDFAIL
         updateRating(choice, positive, false, reduction());
      };
   }

public:
   template <class Vars>
   FDS(CPSolver::Ptr cp, Vars const& starts, double alpha = 0.95)
      : _cp(cp), _x(starts.begin(), starts.end()), _alpha(alpha),
        _baseDepth(cp->getStateManager()->depth()), _sizeBefore(_x.size()) {
      for (int i = 0; i < (int)_x.size(); i++) addSplits(i, _x[i]->min(), _x[i]->max());
   }

   Branches next() {
      if (allFixed()) return Branches({});
      for (;;) {
         int best = -1;
         double bestRating = 0.0;
         for (int k = 0; k < (int)_choices.size(); k++) {
            if (resolved(_choices[k])) continue;
            double const r = _choices[k].rating();
            if (best < 0 || r < bestRating) best = k, bestRating = r;
         }
         if (best >= 0) {
            // the branch with the lower rating first: it is the more likely to fail
            bool const positiveFirst = _choices[best].pos <= _choices[best].neg;
            return branch(best, positiveFirst) | branch(best, !positiveFirst);
         }
         addMoreChoices();   // every choice is resolved but some start is not fixed
      }
   }
};

// The FDS branching over the given start variables (see above).
template <class Vars>
inline std::function<Branches(void)> fds(CPSolver::Ptr cp, Vars const& starts, double alpha = 0.95) {
   auto search = std::make_shared<FDS>(cp, starts, alpha);
   return [search]() { return search->next(); };
}
