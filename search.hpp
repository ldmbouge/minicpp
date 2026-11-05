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

class Branches {
   std::vector<std::function<void(void)>> _alts;
public:
   Branches(const Branches& b) : _alts(std::move(b._alts)) {}
   Branches(std::vector<std::function<void(void)>> alts) : _alts(alts) {}
   Branches(std::initializer_list<std::function<void(void)>> alts) { _alts.insert(_alts.begin(),alts.begin(),alts.end());}
   ~Branches() {}
   std::vector<std::function<void(void)>>::iterator begin() { return _alts.begin();}
   std::vector<std::function<void(void)>>::iterator end()   { return _alts.end();}
   size_t size() const { return _alts.size();}
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
      enum Phase { Fresh = 0, Running = 1, Finished = 2 };
      struct State {
         std::vector<int>         values;    // elements of S, in iteration order
         std::vector<trail<bool>> used;      // done with, or dropped on entry
         trail<int>               current;   // index being processed, -1 if none
         trail<int>               phase;     // Fresh, Running or Finished
         State(Trailer::Ptr sm, const Set& S) : current(sm, -1), phase(sm, Fresh) {
            for (auto v : S) values.push_back(v);
            used.assign(values.size(), trail<bool>(sm, false));   // never resized
         }
      };
      using Key = std::decay_t<decltype(order(0))>;
      auto st = std::make_shared<State>(cp->getStateManager(), S);
      return [st, suchThat, order, body]() -> Branches {
         int const n = (int)st->values.size();
         if (st->phase == Finished)
            return Branches({});
         if (st->phase == Fresh) {           // entering the loop
            for (int k = 0; k < n; k++)
               if (!suchThat(st->values[k])) st->used[k] = true;
            st->phase = Running;
         }
         for (;;) {
            int const c = st->current;
            if (c >= 0) {
               Branches b = body(st->values[c]);
               if (b.size() != 0)
                  return b;
               st->used[c] = true;            // body(values[c]) is done
               st->current = -1;
            }
            int next = -1;
            std::optional<Key> best;
            for (int k = 0; k < n; k++) {
               if (st->used[k] || !suchThat(st->values[k])) continue;
               Key key = order(st->values[k]);
               if (!best || key < *best) { best = key; next = k; }
            }
            if (next < 0) {
               st->phase = Finished;
               return Branches({});
            }
            st->current = next;
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
