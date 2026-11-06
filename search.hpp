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

#ifndef __SEARCH_H
#define __SEARCH_H

#include <vector>
#include <initializer_list>
#include <functional>
#include <iostream>
#include <iomanip>
#include <queue>
#include <optional>
#include <memory>
#include <type_traits>

#include "solver.hpp"
#include "constraint.hpp"
#include "RuntimeMonitor.hpp"
#include "commandList.hpp"
#include <utils.hpp>
#include <cassert>

class Branches {
   std::vector<std::function<void(void)>>         _alts;
   std::function<std::function<void(void)>(void)> _next;   // set only for unbounded Branches
public:
   Branches(const Branches& b) : _alts(std::move(b._alts)), _next(b._next) {}
   Branches(std::vector<std::function<void(void)>> alts) : _alts(alts) {}
   Branches(std::initializer_list<std::function<void(void)>> alts) { _alts.insert(_alts.begin(),alts.begin(),alts.end());}
   ~Branches() {}
   std::vector<std::function<void(void)>>::iterator begin() { return _alts.begin();}
   std::vector<std::function<void(void)>>::iterator end()   { return _alts.end();}
   size_t size() const { return _alts.size();}
   /**
    * Branches whose alternatives are produced on demand: DFS calls next() for
    * each alternative and stops when it returns an empty function (or when the
    * search limit is reached). Used by repeat(...), whose series has no bound.
    */
   // An unbounded Branches holds one placeholder alternative, so that the
   // combinators' "done" test (size() == 0) correctly sees it as non-empty.
   // DFS never runs the placeholder: it pulls alternatives with next() instead.
   static Branches unbounded(std::function<std::function<void(void)>(void)> next) {
      Branches b(std::vector<std::function<void(void)>>{ [] { failNow(); } });
      b._next = std::move(next);
      return b;
   }
   bool isUnbounded() const { return (bool)_next;}
   std::function<void(void)> next() { return _next();}
};

/*
class Chooser {
   std::function<Branches(void)> _sel;
public:
   Chooser(std::function<Branches(void)> sel) : _sel(sel) {}
   Branches operator()() { return _sel();}
};
*/

class SearchStatistics {
protected:
   int nodes;
   int solutions;
   int failures;
   int intVariables;
   int boolVariables;
   int propagators;
   unsigned long long propagations;
   RuntimeMonitor::HRClock _startTime;
   RuntimeMonitor::HRClock _initTime;
   RuntimeMonitor::HRClock _solveTime;
   bool completed;

public:
   SearchStatistics() :
      nodes(0),
      solutions(0),
      failures(0),
      intVariables(0),
      boolVariables(0),
      propagators(0),
      propagations(0),
      completed(true)
   {
      _initTime = _startTime = RuntimeMonitor::now();
   }
   void incrFailures()  noexcept {failures += 1; extern int __nbf; __nbf = failures; }
   void incrNodes()     noexcept {nodes += 1; extern int __nbn; __nbn = nodes;}
   void incrSolutions() noexcept {solutions += 1;}
   void setIntVars(int count) noexcept { intVariables = count;}
   void setBoolVars(int count) noexcept { boolVariables = count;}
   void setPropagators(int count) noexcept {propagators = count;}
   void setInitTime() noexcept { _initTime = RuntimeMonitor::now();}
   void setSolveTime() noexcept { _solveTime = RuntimeMonitor::now();}
   void setPropagations(unsigned long long p) noexcept {propagations = p;}
   void setNotCompleted() noexcept { completed = false;}
   bool getCompleted() noexcept {return completed;}
   int getSolutions() const noexcept {return solutions;}
   int numberOfSolutions() const noexcept {return solutions;}
   int numberOfFailures() const noexcept { return failures;}
   int numberOfNodes() const noexcept { return nodes;}
   // A copy of the counters whose clock starts now: the baseline of a scoped limit.
   SearchStatistics snapshot() const {
      SearchStatistics s(*this);
      s._startTime = s._initTime = RuntimeMonitor::now();
      return s;
   }
   // Moves the counters of a baseline forward by those of d (time untouched).
   void advance(const SearchStatistics& d) {
      nodes     += d.nodes;
      failures  += d.failures;
      solutions += d.solutions;
   }
   // Counters accumulated since base, timed from base's clock: what a scoped limit sees.
   SearchStatistics since(const SearchStatistics& base) const {
      SearchStatistics s(*this);
      s.nodes     -= base.nodes;
      s.failures  -= base.failures;
      s.solutions -= base.solutions;
      s._startTime = s._initTime = base._startTime;
      return s;
   }
   RuntimeMonitor::HRClock getStartTime() const noexcept {return _startTime;}
   RuntimeMonitor::HRClock startTime() const noexcept {return _startTime;}
   friend std::ostream& operator<<(std::ostream& os,const SearchStatistics& ss) {
      return os
         << std::fixed << std::setprecision(3)
         << "Init Time = " << RuntimeMonitor::elapsedSeconds(ss._startTime, ss._initTime) << std::endl
         << "Solve Time = " << RuntimeMonitor::elapsedSeconds(ss._initTime, ss._solveTime) << std::endl
         << "Solutions = " << ss.solutions << std::endl
         << "Variables = " << ss.intVariables + ss.boolVariables << std::endl
         << "Constraints = " << ss.propagators << std::endl
         << "Propagations = " << ss.propagations << std::endl
         << "Nodes = " << ss.nodes << std::endl
         << "Failures = " << ss.failures << std::endl;
   }
};

typedef std::function<bool(const SearchStatistics&)> Limit;

class DFSearch {
   StateManager::Ptr                      _sm;
   CPSolver::Ptr                          _cp;
   std::function<Branches(void)>   _branching;
   std::vector<std::function<void(void)>>    _solutionListeners;
   std::vector<std::function<void(void)>>    _failureListeners;
   void dfs(SearchStatistics& stats,const Limit& limit);
   void dfsUnbounded(Branches& branches,SearchStatistics& stats,const Limit& limit);
public:
   DFSearch(CPSolver::Ptr cp,std::function<Branches(void)>&& b)
      : _sm(cp->getStateManager()),_cp(cp),_branching(std::move(b)) {
      _sm->enable();
   }
   // DFSearch(StateManager::Ptr sm,std::function<Branches(void)>&& b)
   //    : _sm(sm),_branching(std::move(b)) {
   //    _sm->enable();
   // }
   template <class B> void onSolution(B c) { _solutionListeners.emplace_back(std::move(c));}
   template <class B> void onFailure(B c)  { _failureListeners.emplace_back(std::move(c));}
   void notifySolution() { for_each(_solutionListeners.begin(),_solutionListeners.end(),[](std::function<void(void)>& c) { c();});}
   void notifyFailure()  { for_each(_failureListeners.begin(),_failureListeners.end(),[](std::function<void(void)>& c) { c();});}
   SearchStatistics solve(SearchStatistics& stat,Limit limit);
   SearchStatistics solve(Limit limit);
   SearchStatistics solve();
   SearchStatistics solveSubjectTo(Limit limit,std::function<void(void)> subjectTo);
   SearchStatistics optimize(Objective::Ptr obj,SearchStatistics& stat,Limit limit);
   SearchStatistics optimize(Objective::Ptr obj,SearchStatistics& stat);
   SearchStatistics optimize(Objective::Ptr obj,Limit limit);
   SearchStatistics optimize(Objective::Ptr obj);
   SearchStatistics optimizeSubjectTo(Objective::Ptr obj,Limit limit,std::function<void(void)> subjectTo);
};

class BFSNode {
public:
   std::shared_ptr<Checkpoint> _checkpoint;
   int _objectiveValue;
   int _depth;
   BFSNode() { }
   BFSNode(std::shared_ptr<Checkpoint> checkpoint, int objectiveValue, int depth)
      : _checkpoint(checkpoint), _objectiveValue(objectiveValue), _depth(depth) { }
};

struct BFSNodeCompare
{
   bool _minimize;
   BFSNodeCompare(bool minimize = 0) : _minimize(minimize) {}
   bool operator()(const BFSNode& lhs, const BFSNode& rhs) const {
      return (_minimize ? lhs._objectiveValue > rhs._objectiveValue : lhs._objectiveValue < rhs._objectiveValue) || (lhs._objectiveValue == rhs._objectiveValue && lhs._depth < rhs._depth);
   }
};

class BFSearch {
   Trailer::Ptr                      _sm;
   CPSemSolver::Ptr                          _cp;
   std::function<Branches(void)>   _branching;
   std::vector<std::function<void(void)>>    _solutionListeners;
   std::vector<std::function<void(void)>>    _failureListeners;
   Objective::Ptr _objective;
   void bfs(SearchStatistics& stats,const Limit& limit);
   std::shared_ptr<Checkpoint> _before;
public:
   BFSearch(CPSemSolver::Ptr cp,std::function<Branches(void)>&& b)
      : _sm(cp->getStateManager()),_cp(cp),_branching(std::move(b)) {
      _sm->enable();
   }
   SearchStatistics solve(SearchStatistics& stat,Limit limit);
   SearchStatistics solve(Limit limit);
   SearchStatistics solve();
   template <class B> void onSolution(B c) { _solutionListeners.emplace_back(std::move(c));}
   template <class B> void onFailure(B c)  { _failureListeners.emplace_back(std::move(c));}
   void notifySolution() { for_each(_solutionListeners.begin(),_solutionListeners.end(),[](std::function<void(void)>& c) { c();});}
   void notifyFailure()  { for_each(_failureListeners.begin(),_failureListeners.end(),[](std::function<void(void)>& c) { c();});}
   SearchStatistics optimize(Objective::Ptr obj,SearchStatistics& stat,Limit limit);
   SearchStatistics optimize(Objective::Ptr obj,SearchStatistics& stat);
   SearchStatistics optimize(Objective::Ptr obj,Limit limit);
   SearchStatistics optimize(Objective::Ptr obj);
};

template<class B>
std::function<Branches(void)> land(std::initializer_list<B> allB) {
   std::vector<B> vec(allB);
   return land(vec);
}

template<class B>
std::function<Branches(void)> land(std::vector<B> vec)
{
   return [vec]()
   {
      for(size_t i = 0; i < vec.size(); i += 1)
         {
            auto br = vec[i]();
            if (br.size() != 0)
               return br;
         }
      return Branches({});
   };
}

/**
 * Variadic land: land(b1, b2, ..., bn) for any callables `void -> Branches`
 * (std::function, lambdas, the result of forall(...), ...). Same meaning as
 * land({b1, ..., bn}): at each node, the first b_i that produces branches
 * wins. Unlike the initializer-list form, the arguments need not share a
 * type, so lambdas can be passed directly.
 */
template <class B, class... Bs,
          std::enable_if_t<std::is_invocable_r_v<Branches, B&> &&
                           (std::is_invocable_r_v<Branches, Bs&> && ...), int> = 0>
std::function<Branches(void)> land(B b, Bs... bs)
{
   std::vector<std::function<Branches(void)>> vec {
      std::function<Branches(void)>(std::move(b)),
      std::function<Branches(void)>(std::move(bs))...
   };
   return land(std::move(vec));
}

inline Branches operator|(std::function<void(void)> b0, std::function<void(void)> b1) {
   return Branches({ b0,b1 });
}

namespace detail {
   /**
    * Shared implementation of the scoped limits (limitFailures, limitTime).
    * The scope is the subtree below the first node where the closure runs.
    * Entering it records a baseline of the search statistics and registers the
    * limit with the solver, so that repeat(...) sees it. `lim` is an ordinary
    * Limit, evaluated on the statistics accumulated since the scope was entered.
    * Only what happens inside the scope counts: after body is done, the
    * search that follows it is outside. Once the limit holds, every remaining
    * node of the scope fails and the search resumes outside the scope (the search-wide Limit given to solve/optimize,
    * in contrast, stops the whole search). Once body is done in a subtree, the
    * limit no longer applies there. The scope markers are trailed, so leaving
    * and re-entering the scope (e.g., the next LNS iteration) starts afresh.
    * Named detail::limit because solve/optimize call their Limit parameter
    * `limit`, which would hide a global function of that name.
    */
   template <class Body>
   std::function<Branches(void)> limit(CPSolver::Ptr cp, Limit lim, Body body)
   {
      class State {
         trail<bool>      _entered;    // this subtree is inside the scope
         trail<bool>      _done;       // body completed in this subtree
         SearchStatistics _base;       // the scope's counters are measured from here
         SearchStatistics _pausedAt;   // statistics when body last completed
         bool             _paused = false;
         // Counts made after body completed happened outside the scope. When
         // the search comes back into the scope, move the baseline past them,
         // so that the limit only counts what happened inside (as objcp does).
         void resume(const SearchStatistics& now) {
            if (!_paused) return;
            _base.advance(now.since(_pausedAt));
            _paused = false;
         }
      public:
         State(Trailer::Ptr sm) : _entered(sm, false), _done(sm, false) {}
         bool isEntered() const { return _entered; }
         bool isDone() const { return _done; }
         void enter(const SearchStatistics& now) {
            _entered = true;
            _base    = now.snapshot();
            _paused  = false;
         }
         void complete(const SearchStatistics& now) {
            _done     = true;
            _pausedAt = now;
            _paused   = true;
         }
         bool isExceeded(const Limit& lim, const SearchStatistics& now) {
            if (_done) return false;
            resume(now);
            return lim(now.since(_base));
         }
      };
      auto st = std::make_shared<State>(cp->getStateManager());
      auto isExceeded = [cp, st, lim]() -> bool {
         return st->isExceeded(lim, *cp->searchStatistics());
      };
      return [cp, st, isExceeded, body]() -> Branches {
         if (st->isDone())
            return Branches({});
         assert(cp->searchStatistics() && "scoped limits must run inside a DFSearch");
         if (!st->isEntered()) {
            st->enter(*cp->searchStatistics());
            cp->enterLimit(isExceeded);
         } else if (isExceeded())
            return Branches({ [] { failNow(); } });   // fail inside dfs, not across this call
         Branches b = body();
         if (b.size() == 0)
            st->complete(*cp->searchStatistics());
         return b;
      };
   }
}

/**
 * Objective-CP's `limitFailures: n in: body`: once body's subtree has met n
 * failures, the rest of that subtree is abandoned and the search resumes
 * outside it. Re-entering the scope (e.g., the next repeat iteration) starts a
 * new count.
 */
template <class Body>
std::function<Branches(void)> limitFailures(CPSolver::Ptr cp, int n, Body body)
{
   return detail::limit(cp, [n](const SearchStatistics& s) { return s.numberOfFailures() >= n; },
                        std::move(body));
}

/**
 * Objective-CP's `limitTime: t in: body`, with t in seconds: once body's
 * subtree has run for t seconds, the rest of it is abandoned and the search
 * resumes outside it. To bound a whole search, pass a Limit to solve/optimize.
 */
template <class Body>
std::function<Branches(void)> limitTime(CPSolver::Ptr cp, double seconds, Body body)
{
   return detail::limit(cp, [seconds](const SearchStatistics& s) {
                           return RuntimeMonitor::elapsedSeconds(s.startTime()) >= seconds; },
                        std::move(body));
}

/**
 * Objective-CP's `repeat: body onRepeat: onRepeat until: until`. At the node
 * where it is entered, it offers an unbounded series of alternatives, all
 * starting from the state at that node: the first runs body, and each later
 * one runs onRepeat() and then body. Within an iteration, body runs until it
 * reports done (empty Branches). The series stops once until() holds or an
 * enclosing scoped limit (limitFailures, limitTime) has fired; the Limit given
 * to solve/optimize also stops it. onRepeat may post constraints (e.g., to
 * freeze part of a solution); if that fails, the next iteration starts.
 * Typically body is wrapped in limitFailures, so that each iteration is bounded.
 */
template <class Body, class OnRepeat, class Until>
std::function<Branches(void)> repeat(CPSolver::Ptr cp, Body body, OnRepeat onRepeat, Until until)
{
   class State {
      enum Phase { Fresh = 0, Running = 1, Done = 2 };
      trail<int> _phase;           // Fresh at the repeat node, then Running/Done in an iteration
      int        _iteration = 0;   // not trailed: iterations started so far
   public:
      State(Trailer::Ptr sm) : _phase(sm, Fresh) {}
      bool isFresh() const { return _phase == Fresh; }
      bool isDone() const { return _phase == Done; }
      void run() { _phase = Running; }
      void complete() { _phase = Done; }
      int  nextIteration() { return _iteration++; }
   };
   auto st = std::make_shared<State>(cp->getStateManager());
   return [cp, st, body, onRepeat, until]() -> Branches {
      if (st->isDone())
         return Branches({});
      if (!st->isFresh()) {                 // inside an iteration
         Branches b = body();
         if (b.size() == 0)
            st->complete();
         return b;
      }
      return Branches::unbounded([cp, st, onRepeat, until]() -> std::function<void(void)> {
         if (until() || cp->isAborted())
            return nullptr;
         int const k = st->nextIteration();
         return [st, onRepeat, k] {
            st->run();
            if (k > 0) onRepeat();
         };
      });
   };
}

/** repeat(cp, body, onRepeat) with no `until`: runs until a limit stops it. */
template <class Body, class OnRepeat>
std::function<Branches(void)> repeat(CPSolver::Ptr cp, Body body, OnRepeat onRepeat)
{
   return repeat(cp, std::move(body), std::move(onRepeat), [] { return false; });
}

/**
 * Large neighbourhood search, Objective-CP's LNS template:
 *
 *    repeat: { limit: { search } }
 *    onRepeat: { freeze a fragment of the best solution }
 *
 * Each iteration runs `search` from the state at the lns node, under
 * `iterationLimit` (an ordinary Limit, evaluated on what happens inside the
 * iteration only; e.g. a failure count). Whenever `search` reaches a solution
 * (it reports done with empty Branches), `capture()` is called first, so that
 * it can record the solution; DFS then notifies the solution as usual, which
 * tightens the objective under optimize. From the second iteration on,
 * `freeze()` runs before `search` and may post constraints that fix part of the
 * captured solution; until a first solution has been captured, freeze is
 * skipped and the iteration searches from scratch.
 *
 * lns is meant to be the whole search (its leaves are the solutions). It runs
 * until stopped from outside: use the Limit given to optimize, or wrap it in a
 * scoped limit. Without an iteration limit, every iteration is a complete
 * search of its neighbourhood.
 *
 * @param cp             the solver
 * @param capture        `void()`: record the current (complete) solution
 * @param freeze         `void()`: post constraints fixing a fragment of the
 *                       last captured solution; a failure starts the next iteration
 * @param search         the core search, a branching `void -> Branches`
 * @param iterationLimit optional: bounds each iteration
 */
template <class Capture, class Freeze, class Search>
std::function<Branches(void)> lns(CPSolver::Ptr cp, Capture capture, Freeze freeze,
                                  Search search, Limit iterationLimit)
{
   auto hasSolution = std::make_shared<bool>(false);
   std::function<Branches(void)> body = [search, capture, hasSolution]() -> Branches {
      Branches b = search();
      if (b.size() == 0) {          // a leaf: the solution DFS is about to report
         capture();
         *hasSolution = true;
      }
      return b;
   };
   if (iterationLimit)
      body = detail::limit(cp, iterationLimit, std::move(body));
   return repeat(cp, std::move(body), [freeze, hasSolution] {
      if (*hasSolution) freeze();
   });
}

/** lns with no iteration limit. */
template <class Capture, class Freeze, class Search>
std::function<Branches(void)> lns(CPSolver::Ptr cp, Capture capture, Freeze freeze, Search search)
{
   return lns(cp, std::move(capture), std::move(freeze), std::move(search), Limit());
}

namespace detail {
   /**
    * Shared implementation of the forall combinators (see forall below).
    * Mirrors ORControl's forall:suchThat:orderedBy:do: loop:
    *  - on entry, elements failing suchThat are dropped for good;
    *  - then, repeatedly: among the elements not done that pass suchThat, take
    *    the one with the smallest order key (ties: earliest in S), and run its
    *    body until it reports done (empty Branches);
    *  - the loop ends, for good, as soon as no element qualifies.
    * The loop's state (entered/finished, current element, elements done) is
    * trailed so that it follows the search tree.
    */
   template <class Set, class Filter, class Order, class Body>
   std::function<Branches(void)> forall(CPSolver::Ptr cp, const Set& S,
                                        Filter suchThat, Order order, Body body)
   {
      using Key = std::decay_t<decltype(order(0))>;
      // The loop's trailed state. _used is filled once and never resized, so
      // the addresses recorded on the trail stay valid.
      class State {
         enum Phase { Fresh = 0, Running = 1, Finished = 2 };
         std::vector<int>         _values;    // elements of S, in iteration order
         std::vector<trail<bool>> _used;      // done with, or dropped on entry
         trail<int>               _current;   // index being processed, -1 if none
         trail<int>               _phase;     // Fresh, Running or Finished
      public:
         State(Trailer::Ptr sm, const Set& S) : _current(sm, -1), _phase(sm, Fresh) {
            for (auto v : S) _values.push_back(v);
            _used.assign(_values.size(), trail<bool>(sm, false));
         }
         bool isFinished() const { return _phase == Finished; }
         // On first entry, drop for good the elements failing suchThat.
         void enter(const Filter& suchThat) {
            if (_phase != Fresh) return;
            for (int k = 0; k < (int)_values.size(); k++)
               if (!suchThat(_values[k])) _used[k] = true;
            _phase = Running;
         }
         bool hasCurrent() const { return _current >= 0; }
         int  currentValue() const { return _values[_current]; }
         // body(currentValue()) is done.
         void completeCurrent() {
            _used[_current] = true;
            _current = -1;
         }
         // Make the qualifying element with the smallest key current (ties:
         // earliest in S). Returns false, and finishes the loop, if none.
         bool selectNext(const Filter& suchThat, const Order& order) {
            int next = -1;
            std::optional<Key> best;
            for (int k = 0; k < (int)_values.size(); k++) {
               if (_used[k] || !suchThat(_values[k])) continue;
               Key key = order(_values[k]);
               if (!best || key < *best) { best = key; next = k; }
            }
            if (next < 0) {
               _phase = Finished;
               return false;
            }
            _current = next;
            return true;
         }
      };
      auto st = std::make_shared<State>(cp->getStateManager(), S);
      return [st, suchThat, order, body]() -> Branches {
         if (st->isFinished())
            return Branches({});
         st->enter(suchThat);
         for (;;) {
            if (st->hasCurrent()) {
               Branches b = body(st->currentValue());
               if (b.size() != 0)
                  return b;
               st->completeCurrent();
            }
            if (!st->selectNext(suchThat, order))
               return Branches({});
         }
      };
   }
}

/**
 * Search combinator equivalent to Objective-CP's `forall: S do: body`
 * (ORControl). It runs body(i) for every i in S, one element at a time and in
 * the iteration order of S: the current element's body keeps producing
 * branches until it reports that it is done (empty Branches), and only then
 * does the next element start.
 *
 * The combinator's state (current element, elements done) is trailed, so it
 * follows the search: backtracking above the point where an element was
 * started or completed restores that state, as re-running the Objective-CP
 * loop would.
 *
 * @param cp   the solver (its state manager trails the combinator's state)
 * @param S    any iterable of int (a std::vector<int>, a range(...), ...)
 * @param body a function `int -> Branches`; empty Branches means "done with i"
 * @return a branching usable with DFSearch, or composable with land(...)
 */
template <class Set, class Body>
std::function<Branches(void)> forall(CPSolver::Ptr cp, const Set& S, Body body)
{
   return detail::forall(cp, S, [](int) { return true; }, [](int) { return 0; }, body);
}

/**
 * Equivalent to Objective-CP's `forall: S orderedBy: order do: body`.
 * Each time an element is done, the next one is the element of S not done yet
 * with the smallest order(i), evaluated in the current state (ties: earliest
 * in S). The key can be of any type with operator< (e.g. a std::pair or
 * std::tuple for `orderedBy: o1 then: o2`).
 */
template <class Set, class Order, class Body>
std::function<Branches(void)> forall(CPSolver::Ptr cp, const Set& S, Order order, Body body)
{
   return detail::forall(cp, S, [](int) { return true; }, order, body);
}

/**
 * Equivalent to Objective-CP's `forall: S suchThat: suchThat orderedBy: order do: body`.
 * Like the ordered forall, but only elements i with suchThat(i) are considered:
 * elements failing it on entry are dropped, later ones are skipped while they
 * fail it, and the loop ends as soon as no remaining element passes it.
 */
template <class Set, class Filter, class Order, class Body>
std::function<Branches(void)> forall(CPSolver::Ptr cp, const Set& S, Filter suchThat, Order order, Body body)
{
   return detail::forall(cp, S, suchThat, order, body);
}


namespace detail {
   /**
    * Value selection of Objective-CP's selectValue:by:then: (CPSolver): the
    * value v in the domain of x with the smallest o1(v), ties broken by the
    * smallest o2(v), remaining ties by the smallest v. Keys can be of any type
    * with operator<.
    */
   template <class Var, class O1, class O2>
   int selectValue(const Var& x, O1& o1, O2& o2)
   {
      using K1 = std::decay_t<decltype(o1(0))>;
      using K2 = std::decay_t<decltype(o2(0))>;
      std::optional<K1> best1;
      std::optional<K2> best2;
      int chosen = x->min();
      for (int v = x->min(); v <= x->max(); v++) {
         if (!x->contains(v)) continue;
         K1 k1 = o1(v);
         if (!best1 || k1 < *best1) {
            best1 = k1;
            best2 = o2(v);
            chosen = v;
         } else if (!(*best1 < k1)) {          // tie on o1
            K2 k2 = o2(v);
            if (k2 < *best2) { best2 = k2; chosen = v; }
         }
      }
      return chosen;
   }
   template <class Var>
   Branches labelWith(CPSolver::Ptr cp, Var x, int v)
   {
      using namespace Factory;
      return [cp, x, v] { cp->post(x == v); }
           | [cp, x, v] { cp->post(x != v); };
   }
}

/**
 * One step of Objective-CP's `label: x`: while x is unbound, try x == min(x),
 * else x != min(x). Returns empty Branches once x is bound, so it composes
 * with forall(...) and land(...).
 */
template <class Var>
Branches label(CPSolver::Ptr cp, Var x)
{
   if (x->isBound()) return Branches({});
   return detail::labelWith(cp, x, x->min());
}

/**
 * One step of Objective-CP's `label: x by: o`: while x is unbound, try the
 * value v with the smallest o(v) (ties: smallest v), else remove it.
 * Returns empty Branches once x is bound.
 */
template <class Var, class O1>
Branches label(CPSolver::Ptr cp, Var x, O1 o1)
{
   if (x->isBound()) return Branches({});
   auto none = [](int) { return 0; };
   return detail::labelWith(cp, x, detail::selectValue(x, o1, none));
}

/**
 * One step of Objective-CP's `label: x by: o1 then: o2`: while x is unbound,
 * try the value v with the smallest o1(v), ties broken by the smallest o2(v),
 * else remove it. Returns empty Branches once x is bound.
 */
template <class Var, class O1, class O2>
Branches label(CPSolver::Ptr cp, Var x, O1 o1, O2 o2)
{
   if (x->isBound()) return Branches({});
   return detail::labelWith(cp, x, detail::selectValue(x, o1, o2));
}

namespace detail {
   // First successor on the path from node 0 that is still unbound, or -1 once
   // the path reaches the sink (node succ.size()).
   template <class Vec>
   int sequenceFrontier(const Vec& succ)
   {
      int const sink = (int)succ.size();
      if (sink == 0) return -1;              // nothing to sequence
      int k = 0;
      for (int steps = 0; steps <= sink; steps++) {
         if (!succ[k]->isBound()) return k;
         k = succ[k]->min();
         if (k >= sink) return -1;
      }
      return -1;   // a cycle: ruled out by the path/circuit constraint
   }
}

/**
 * One step of Objective-CP's `sequence: succ by: o1 then: o2`: on the path
 * 0 -> succ[0] -> succ[succ[0]] -> ... , label the first unbound successor
 * with label(cp, succ[k], o1, o2). Returns empty Branches once the path is
 * complete (every successor on it bound up to the sink, node succ.size()).
 * Nodes are the values of the successor variables, as in TaskSequence.
 */
template <class Vec, class O1, class O2>
Branches sequence(CPSolver::Ptr cp, const Vec& succ, O1 o1, O2 o2)
{
   int const k = detail::sequenceFrontier(succ);
   if (k < 0) return Branches({});
   return label(cp, succ[k], o1, o2);
}

/**
 * One step of Objective-CP's `sequence: succ by: o`: as above with a single
 * value ordering.
 */
template <class Vec, class O1>
Branches sequence(CPSolver::Ptr cp, const Vec& succ, O1 o1)
{
   int const k = detail::sequenceFrontier(succ);
   if (k < 0) return Branches({});
   return label(cp, succ[k], o1);
}

template<class Container,typename Predicate, typename Fun>
typename Container::value_type selectMin(const Container& c,Predicate test, Fun f,
                                         typename Container::value_type def = typename Container::value_type())
{
   auto from = c.begin();
   auto to = c.end();
   auto min = to;
   for(; from != to; from++)
      {
         if (test(*from))
            {
               auto fv = f(*from);
               if (min == to || fv < f(*min))
                  min = from;
            }
      }
   if (min == to)
      return def;
   else 
      return *min;
}

template<class Container,typename Predicate, typename Fun>
typename Container::value_type selectMin3(const Container& c,Predicate test, Fun f) {
   return selectMin(c,test,f,typename Container::value_type());
}

template<class Container,typename Predicate>
typename Container::value_type selectFirst(const Container& c,Predicate test)
{
   auto from = c.begin();
   auto to = c.end();
   for(; from != to; from++) {
      if (test(*from))
         return *from;
   }
   return typename Container::value_type();
}


template <class Container> std::function<Branches(void)> firstFail(CPSolver::Ptr cp,const Container& c) {
   using namespace Factory;
   return [=]() {
      auto sx = selectMin(c,
                          [](const auto& x) { return x->size() > 1;},
                          [](const auto& x) { return x->size();});
      if (sx) {
         int v = sx->min();
         return [cp,sx,v] { return cp->post(sx == v);}
            |  [cp,sx,v] { return cp->post(sx != v);};
      } else return Branches({});
   };
}

template <class Container> std::function<Branches(void)> smallest(CPSolver::Ptr cp,const Container& c) {
   using namespace Factory;
   return [=]() {
      auto sx = selectMin(c,
                          [](const auto& x) { return x->size() > 1;},
                          [](const auto& x) { return x->min();});
      if (sx) {
         int v = sx->min();
         return [cp,sx,v] { return cp->post(sx == v);}
            |  [cp,sx,v] { return cp->post(sx != v);};
      } else return Branches({});
   };
}

template <class Container,typename Predicate, typename Fun>
typename Container::value_type selectMax(const Container& c,Predicate test, Fun f,
                                         typename Container::value_type def = typename Container::value_type())
{
   auto from = c.begin();
   auto to = c.end();
   auto max = to;
   for(; from != to; from++) {
      if (test(*from)) {
         auto fv = f(*from);
         if (max == to || fv > f(*max))
            max = from;
      }
   }
   if (max == to)
      return def;
   else
      return *max;
}


// Unused: Var cannot be deduced, so callers would have to name it explicitly.
// Superseded by the branchings firstFail(cp, vars) and smallest(cp, vars)
// (largest has no branching counterpart).
/*
template<class Vars, class Var>
Var first_fail(Vars const & vars) {
   return selectMin(
                    vars,
                    [](const auto& x) { return x->size() > 1;},
                    [](const auto& x) { return x->size();}
                    );
}

template<class Vars, class Var>
Var input_order(Vars const & vars) {
   return selectMin(
                    vars,
                    [](const auto& x) { return x->size() > 1;},
                    [&](const auto& x)
                    {
                       auto it = std::find(vars.begin(), vars.end(), x);
                       return std::distance(vars.begin(), it);
                    }
                    );
}

template<class Vars, class Var>
Var smallest(Vars const & vars) {
   return selectMin(
                    vars,
                    [](const auto& x) { return x->size() > 1;},
                    [](const auto& x) { return x->min();}
                    );
}

template<class Vars, class Var>
Var largest(Vars const & vars) {
   return selectMax(
                    vars,
                    [](const auto& x) { return x->size() > 1;},
                    [](const auto& x) { return x->max();}
                    );
}
*/


// Values selections
template<class Var> Branches indomain_min(CPSolver::Ptr cp, Var var) {
   using namespace Factory;
   if (var) {
      auto val = var->min();
      return [cp,var,val] {
         TRACE(std::cout << "Choosing x" << var->getId() << " == "<< val << std::endl;);
         return cp->post(var == val);
      } |
         [cp,var,val] {
            TRACE(std::cout << "Choosing x" << var->getId() << " != "<< val << std::endl;);
            return cp->post(var != val);
         };
   } else
      return Branches({});
}

template <class Var> Branches indomain_max(CPSolver::Ptr cp, Var var) {
   using namespace Factory;

   if (var) {
      auto val = var->max();
      return [cp,var,val] {
         TRACE(std::cout << "Choosing x" << var->getId() << " == "<< val << std::endl;);
         return cp->post(var == val);
      } |
         [cp,var,val] {
            TRACE(std::cout << "Choosing x" << var->getId() << " != "<< val << std::endl;);
            return cp->post(var != val);
         };
   } else
      return Branches({});
}

template <class Var> Branches indomain_split(CPSolver::Ptr cp, Var var) {
   using namespace Factory;

   if (var) {
      auto val = (var->max() + var->min()) / 2;
      return [cp,var,val] {
         TRACE(std::cout << "Choosing x" << var->getId() << " <= "<< val << std::endl;);
         return cp->post(var <= val);
      } |
         [cp,var,val] {
            TRACE(std::cout << "Choosing x" << var->getId() << " > "<< val << std::endl;);
            return cp->post(var >= val + 1);
         };
   } else
      return Branches({});
}


#endif
