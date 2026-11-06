// Tests for the Objective-CP style search combinators in search.hpp:
// forall (plain, orderedBy, suchThat), label (plain, by, by/then), sequence
// and the variadic land.
//
// The forall tests record, per solution, the order in which the forall
// *started* its elements along the path to that solution (kept in trailed
// state, so it is exact even across backtracking).

#include <iostream>
#include <vector>
#include <map>
#include <algorithm>
#include <climits>
#include "solver.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"
#include "trailable.hpp"
#include "range.hpp"
#include "scheduling.hpp"

using namespace Factory;

static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::cerr << "CHECK failed line " << __LINE__ << ": " #c "\n"; failures++; } } while (0)

// Path recorder: which elements were started, in which order, on the current path.
struct Recorder {
   std::vector<trail<int>>  seq;
   std::vector<trail<bool>> started;
   trail<int>               len;
   Recorder(CPSolver::Ptr cp, int n) : len(cp->getStateManager(), 0) {
      seq.assign(n, trail<int>(cp->getStateManager(), -1));
      started.assign(n, trail<bool>(cp->getStateManager(), false));
   }
   void start(int i) {
      if (started[i]) return;
      started[i] = true;
      seq[len] = i;
      len = len + 1;
   }
   std::vector<int> path() const {
      std::vector<int> p;
      for (int k = 0; k < len; k++) p.push_back(seq[k]);
      return p;
   }
};

// body(i): label x[i] from its smallest value up; done once x[i] is bound.
static auto labeler(CPSolver::Ptr cp, Veci& x, Recorder& rec)
{
   return [cp, &x, &rec](int i) -> Branches {
      if (x[i]->isBound()) return Branches({});
      rec.start(i);
      int const v = x[i]->min();
      auto xi = x[i];
      return [cp, xi, v] { cp->post(xi == v); }
           | [cp, xi, v] { cp->post(xi != v); };
   };
}

static void testVanilla()
{
   CPSolver::Ptr cp = makeSolver();
   auto x = intVarArray(cp, 3, 0, 1);
   Recorder rec(cp, 3);
   DFSearch search(cp, forall(cp, range(0, 2), labeler(cp, x, rec)));
   int bad = 0;
   search.onSolution([&] { if (rec.path() != std::vector<int>{0, 1, 2}) bad++; });
   auto stat = search.solve();
   CHECK(stat.numberOfSolutions() == 8);
   CHECK(bad == 0);
}

static void testOrderedBy()
{
   // Ordered by largest min. At the root all mins are 0, so x0 goes first.
   // x1 >= x0 and x2 >= 2*x0: once x0 = 1, min(x2) = 2 > min(x1) = 1, so x2
   // must come before x1; with x0 = 0 the tie keeps x1 first. A forall sorted
   // once at the root would always do x1 before x2.
   CPSolver::Ptr cp = makeSolver();
   auto x = intVarArray(cp, 3, 0, 3);
   cp->post(x[0] <= 1);
   cp->post(x[0] <= x[1]);
   cp->post(x[0] * 2 <= x[2]);
   Recorder rec(cp, 3);
   DFSearch search(cp, forall(cp, range(0, 2),
                              [&x](int i) { return -x[i]->min(); },
                              labeler(cp, x, rec)));
   int bad = 0, sols0 = 0, sols1 = 0;
   search.onSolution([&] {
      auto p = rec.path();
      if (x[0]->min() == 0) { sols0++; if (p != std::vector<int>{0, 1, 2}) bad++; }
      else                  { sols1++; if (p != std::vector<int>{0, 2, 1}) bad++; }
   });
   auto stat = search.solve();
   CHECK(sols0 == 16);        // x1, x2 free in [0,3]
   CHECK(sols1 == 6);         // x1 in [1,3], x2 in [2,3]
   CHECK(stat.numberOfSolutions() == 22);
   CHECK(bad == 0);
}

static void testOrderedByPair()
{
   // orderedBy:then: as a pair key: primary key equal for all, so the second
   // key (largest index first) decides.
   CPSolver::Ptr cp = makeSolver();
   auto x = intVarArray(cp, 3, 0, 1);
   Recorder rec(cp, 3);
   DFSearch search(cp, forall(cp, range(0, 2),
                              [](int i) { return std::make_pair(0, -i); },
                              labeler(cp, x, rec)));
   int bad = 0;
   search.onSolution([&] { if (rec.path() != std::vector<int>{2, 1, 0}) bad++; });
   auto stat = search.solve();
   CHECK(stat.numberOfSolutions() == 8);
   CHECK(bad == 0);
}

static void testSuchThat()
{
   // Only even elements are labeled; odd ones stay unbound in every solution.
   CPSolver::Ptr cp = makeSolver();
   auto x = intVarArray(cp, 4, 0, 1);
   Recorder rec(cp, 4);
   DFSearch search(cp, forall(cp, range(0, 3),
                              [](int i) { return i % 2 == 0; },
                              [](int i) { return i; },
                              labeler(cp, x, rec)));
   int bad = 0;
   search.onSolution([&] {
      if (rec.path() != std::vector<int>{0, 2}) bad++;
      if (x[1]->isBound() || x[3]->isBound()) bad++;
   });
   auto stat = search.solve();
   CHECK(stat.numberOfSolutions() == 4);
   CHECK(bad == 0);
}

static void testSuchThatEndsLoop()
{
   // suchThat(i) = "x[3] is still unbound". Element 3 is the first by order
   // and binds x[3]; after it, nothing passes suchThat, so the loop ends for
   // good and the land() fallback labels the rest. Elements 0..2 are never
   // started by the forall.
   CPSolver::Ptr cp = makeSolver();
   auto x = intVarArray(cp, 4, 0, 1);
   Recorder rec(cp, 4);
   auto loop = forall(cp, range(0, 3),
                      [&x](int) { return !x[3]->isBound(); },
                      [](int i) { return -i; },
                      labeler(cp, x, rec));
   DFSearch search(cp, land({loop, firstFail(cp, x)}));
   int bad = 0;
   search.onSolution([&] { if (rec.path() != std::vector<int>{3}) bad++; });
   auto stat = search.solve();
   CHECK(stat.numberOfSolutions() == 16);
   CHECK(bad == 0);
}

// Applies the first alternative of a step, as DFS would.
static void takeFirst(Branches b) { (*b.begin())(); }

static void testLabel()
{
   CPSolver::Ptr cp = makeSolver();
   cp->getStateManager()->enable();   // normally done by DFSearch; needed for withNewState
   auto x = makeIntVar(cp, 0, 4);
   auto y = makeIntVar(cp, 3, 3);
   CHECK(label(cp, y).size() == 0);                       // bound: done
   CHECK(label(cp, x).size() == 2);
   cp->getStateManager()->withNewState([&] {
      takeFirst(label(cp, x));                             // plain label: min first
      CHECK(x->isBound() && x->min() == 0);
   });
   cp->getStateManager()->withNewState([&] {
      // by: evens first, then: largest first  -> 4
      takeFirst(label(cp, x, [](int v) { return v % 2; }, [](int v) { return -v; }));
      CHECK(x->isBound() && x->min() == 4);
   });
   cp->getStateManager()->withNewState([&] {
      // by: only, with a tie between 1 and 3: smallest value wins -> 1
      takeFirst(label(cp, x, [](int v) { return v % 2 == 0; }));
      CHECK(x->isBound() && x->min() == 1);
   });
   cp->getStateManager()->withNewState([&] {
      // the second alternative removes the chosen value
      auto b = label(cp, x, [](int v) { return -v; });
      (*std::next(b.begin()))();
      CHECK(!x->contains(4) && x->max() == 3);
   });
}

static void testSequence()
{
   // Three unit tasks on one resource: sequencing enumerates the 3! orders.
   CPSolver::Ptr cp = makeSolver();
   std::vector<var<int>::Ptr> s;
   for (int k = 0; k < 3; k++) s.push_back(makeIntVar(cp, 0, 100));
   std::vector<int> d(3, 1);
   std::vector<var<int>::Ptr> succ;
   for (int k = 0; k <= 3; k++) succ.push_back(makeIntVar(cp, 1, 4));
   cp->post(taskSequence(s, d, succ));
   // by: est then: node, so ties go to the lowest task index (sink last)
   auto step = [&] {
      return sequence(cp, succ, [&](int k) { return k <= 3 ? s[k - 1]->min() : INT_MAX; },
                                [](int k) { return k; });
   };
   DFSearch search(cp, step);
   int bad = 0;
   std::vector<int> first;
   search.onSolution([&] {
      for (auto& x : succ) if (!x->isBound()) bad++;
      if (first.empty()) for (auto& x : succ) first.push_back(x->min());
      for (int k = 0; k < 3; k++)   // path order respected by the start times
         if (succ[k + 1]->min() <= 3 && s[k]->min() + 1 > s[succ[k + 1]->min() - 1]->min()) bad++;
   });
   auto stat = search.solve();
   CHECK(stat.numberOfSolutions() == 6);
   CHECK(bad == 0);
   CHECK((first == std::vector<int>{1, 2, 3, 4}));          // 0->1->2->3->sink first
   CHECK(sequence(cp, succ, [](int k) { return k; }).size() == 2);   // root: still a step
}

// land(...) with arguments of different types: a std::function, a forall and
// bare lambdas. The first branching that still has work wins at each node.
template <class T, class = void> struct canLand : std::false_type {};
template <class T>
struct canLand<T, std::void_t<decltype(land(std::declval<T>(), std::declval<T>()))>> : std::true_type {};
static_assert(!canLand<int>::value, "land(...) must reject non-branchings");

static void testLandVariadic()
{
   CPSolver::Ptr cp = makeSolver();
   auto x = intVarArray(cp, 4, 0, 1);
   std::function<Branches(void)> first = [&] { return label(cp, x[0]); };
   Recorder rec(cp, 4);
   DFSearch search(cp, land(first,
                            forall(cp, range(1, 2), labeler(cp, x, rec)),
                            [&] { return label(cp, x[3]); }));
   std::vector<std::vector<int>> sols;
   search.onSolution([&] {
      std::vector<int> v;
      for (auto& xi : x) v.push_back(xi->min());
      sols.push_back(v);
   });
   auto stat = search.solve();
   CHECK(stat.numberOfSolutions() == 16);
   // x0 is the outermost choice, then x1, x2 (forall), then x3 innermost.
   CHECK((sols.front() == std::vector<int>{0, 0, 0, 0}));
   CHECK((sols[1]      == std::vector<int>{0, 0, 0, 1}));
   CHECK((sols[2]      == std::vector<int>{0, 0, 1, 0}));
   CHECK((sols.back()  == std::vector<int>{1, 1, 1, 1}));
}

static void testSequenceResource()
{
   // sequence on a DisjunctiveResource: orderings take task indices; the
   // successor nodes and the sink never show up.
   CPSolver::Ptr cp = makeSolver();
   auto r = disjunctiveResource(cp);
   std::vector<var<int>::Ptr> s;
   for (int t = 0; t < 3; t++) {
      s.push_back(makeIntVar(cp, 0, 100));
      r->add(s[t], 1);
   }
   cp->post(r);
   auto empty = disjunctiveResource(cp);
   cp->post(empty);
   CHECK(sequence(cp, empty, [](int) { return 0; }).size() == 0);   // nothing to do

   // by: est then: largest task index (keys of different types on purpose)
   DFSearch search(cp, [&] {
      return sequence(cp, r, [&](int t) { return r->est(t); },
                             [](int t) { return -(double)t; });
   });
   int bad = 0;
   std::vector<std::vector<int>> orders;   // tasks by start time, per solution
   search.onSolution([&] {
      for (auto& x : r->successors()) if (!x->isBound()) bad++;
      std::vector<int> o{0, 1, 2};
      std::sort(o.begin(), o.end(), [&](int a, int b) { return s[a]->min() < s[b]->min(); });
      orders.push_back(o);
   });
   auto stat = search.solve();
   CHECK(stat.numberOfSolutions() == 6);
   CHECK(bad == 0);
   CHECK((orders.front() == std::vector<int>{2, 1, 0}));   // all est 0: largest index first
   std::sort(orders.begin(), orders.end());
   CHECK(std::unique(orders.begin(), orders.end()) == orders.end());   // 6 distinct orders
}


// ---------------------------------------------------------------------------
// Scoped limits and repeat
// ---------------------------------------------------------------------------

// Pigeonhole: n variables in [0,n-2], pairwise different. Infeasible, but
// forward checking only finds out deep in the tree, after many failures.
static Veci pigeonhole(CPSolver::Ptr cp, int n)
{
   auto y = intVarArray(cp, n, 0, n - 2);
   for (int i = 0; i < n; i++)
      for (int j = i + 1; j < n; j++)
         cp->post(notEqual(y[i], y[j]));
   return y;
}

static void testLimitFailures()
{
   // a in [0,2] is labelled outside the scope; the infeasible pigeonhole
   // search runs inside limitFailures(3). Each value of a re-enters the scope
   // with a fresh count, so every a gets some failures, but fewer than without
   // the limit; the search itself completes normally (the scope only cuts).
   auto run = [](bool limited, std::vector<int>& perA, SearchStatistics& out) {
      CPSolver::Ptr cp = makeSolver();
      auto a = makeIntVar(cp, 0, 2);
      auto y = pigeonhole(cp, 5);
      auto inner = firstFail(cp, y);
      auto body  = limited ? limitFailures(cp, 3, inner) : inner;
      DFSearch search(cp, land(firstFail(cp, std::vector<var<int>::Ptr>{a}), body));
      perA.assign(3, 0);
      search.onFailure([&] { if (a->isBound()) perA[a->min()]++; });
      out = search.solve();
   };
   std::vector<int> unl, lim;
   SearchStatistics su, sl;
   run(false, unl, su);
   run(true, lim, sl);
   CHECK(su.numberOfSolutions() == 0 && sl.numberOfSolutions() == 0);
   CHECK(sl.getCompleted());
   for (int v = 0; v < 3; v++) {
      CHECK(lim[v] >= 3);            // the limit was reached in every scope instance
      CHECK(lim[v] < unl[v]);        // ...and it cut the search
   }
}

static void testLimitDoneScope()
{
   // Once the limited part is done, the limit no longer applies: the search
   // after it (with plenty of failures) is not cut by limitFailures(1).
   auto run = [](bool limited) {
      CPSolver::Ptr cp = makeSolver();
      auto x = makeIntVar(cp, 0, 1);
      auto z = intVarArray(cp, 3, 0, 3);
      cp->post(sum(z, 6));
      for (int i = 0; i < 3; i++)
         for (int j = i + 1; j < 3; j++)
            cp->post(notEqual(z[i], z[j]));
      auto first = firstFail(cp, std::vector<var<int>::Ptr>{x});
      DFSearch search(cp, land(limited ? limitFailures(cp, 1, first) : first, firstFail(cp, z)));
      return search.solve();
   };
   auto su = run(false), sl = run(true);
   CHECK(su.numberOfFailures() > 1);  // the second part does fail
   CHECK(sl.numberOfSolutions() == su.numberOfSolutions());
   CHECK(sl.numberOfFailures() == su.numberOfFailures());
}

static void testRepeat()
{
   // Three iterations over two booleans. onRepeat posts x0 == nRep % 2, so
   // iteration 1 has 4 solutions and iterations 2 and 3 have 2 each; if the
   // posting of iteration 2 (x0 == 1) leaked into iteration 3 (x0 == 0),
   // iteration 3 would have none.
   CPSolver::Ptr cp = makeSolver();
   auto x = intVarArray(cp, 2, 0, 1);
   int nRep = 0;
   std::vector<int> solsPerIter(3, 0);
   DFSearch search(cp, repeat(cp, firstFail(cp, x),
                              [&] { nRep++; cp->post(x[0] == nRep % 2); },
                              [&] { return nRep == 2; }));
   search.onSolution([&] { solsPerIter[nRep]++; });
   auto stat = search.solve();
   CHECK(nRep == 2);
   CHECK(solsPerIter == (std::vector<int>{4, 2, 2}));
   CHECK(stat.numberOfSolutions() == 8);
   CHECK(stat.getCompleted());
}

static void testRepeatLimitedIterations()
{
   // The usual LNS shape: each iteration's body is wrapped in limitFailures.
   // Every iteration starts a fresh count, so each one reaches the limit.
   CPSolver::Ptr cp = makeSolver();
   auto y = pigeonhole(cp, 5);
   int nRep = 0;
   std::vector<int> failsPerIter(4, 0);
   DFSearch search(cp, repeat(cp, limitFailures(cp, 4, firstFail(cp, y)),
                              [&] { nRep++; },
                              [&] { return nRep == 3; }));
   search.onFailure([&] { failsPerIter[nRep]++; });
   auto stat = search.solve();
   CHECK(nRep == 3);
   for (int k = 0; k < 4; k++)
      CHECK(failsPerIter[k] >= 4);
   CHECK(stat.getCompleted());
}

static void testRepeatStopsOnScopedLimit()
{
   // repeat with no `until` inside limitFailures(10): every iteration fails,
   // and once the enclosing scope has 10 failures repeat stops (objcp's
   // isAborted). The search-wide limit is only a guard against looping forever.
   CPSolver::Ptr cp = makeSolver();
   auto y = pigeonhole(cp, 4);
   int nRep = 0;
   DFSearch search(cp, limitFailures(cp, 10, repeat(cp, firstFail(cp, y), [&] { nRep++; })));
   auto stat = search.solve([](const SearchStatistics& s) { return s.numberOfFailures() >= 10000; });
   CHECK(stat.getCompleted());              // not stopped by the guard
   CHECK(stat.numberOfFailures() < 100);
   CHECK(nRep >= 1);
}

static void testRepeatStopsOnSearchLimit()
{
   // repeat with no `until` and no scoped limit: the Limit given to solve stops it.
   CPSolver::Ptr cp = makeSolver();
   auto x = intVarArray(cp, 2, 0, 1);
   int nRep = 0;
   DFSearch search(cp, repeat(cp, firstFail(cp, x), [&] { nRep++; }));
   auto stat = search.solve([](const SearchStatistics& s) { return s.numberOfSolutions() >= 10; });
   CHECK(!stat.getCompleted());
   CHECK(stat.numberOfSolutions() == 10);
   CHECK(nRep == 2);                        // 4 + 4 + 2 solutions
}

static void testLns()
{
   // Minimize the sum of 4 pairwise-different values in [0,9]. The core search
   // tries the largest values first, so the first solution is poor (9+8+7+6).
   // Each iteration is limited to 20 failures and freezes one variable of the
   // last captured solution. Every captured solution must improve on the
   // previous one, and the search-wide limit is what stops lns.
   CPSolver::Ptr cp = makeSolver();
   auto x = intVarArray(cp, 4, 0, 9);
   for (int i = 0; i < 4; i++)
      for (int j = i + 1; j < 4; j++)
         cp->post(notEqual(x[i], x[j]));
   auto total = Factory::sum(x);
   auto obj = minimize(total);
   std::vector<int> best(4), objectives;
   int nFreeze = 0;
   auto core = forall(cp, range(0, 3), [&](int i) { return label(cp, x[i], [](int v) { return -v; }); });
   DFSearch search(cp, lns(cp,
      [&] { for (int i = 0; i < 4; i++) best[i] = x[i]->min();
            objectives.push_back(total->min()); },
      [&] { int const k = nFreeze++ % 4; cp->post(x[k] == best[k]); },
      core,
      [](const SearchStatistics& s) { return s.numberOfFailures() >= 20; }));
   auto stat = search.optimize(obj, [](const SearchStatistics& s) { return s.numberOfNodes() >= 3000; });
   CHECK(!stat.getCompleted());
   CHECK(objectives.size() >= 2);
   CHECK(objectives.front() == 30);
   for (size_t k = 1; k < objectives.size(); k++)
      CHECK(objectives[k] < objectives[k - 1]);
   CHECK(objectives.back() >= 6);
   CHECK(nFreeze >= 1);
}

static void testLnsFreezeNeedsSolution()
{
   // With no solution captured yet, freeze must not run: each iteration
   // searches from scratch (here an infeasible model, cut at 3 failures).
   CPSolver::Ptr cp = makeSolver();
   auto y = pigeonhole(cp, 5);
   int nCapture = 0, nFreeze = 0;
   DFSearch search(cp, lns(cp, [&] { nCapture++; }, [&] { nFreeze++; }, firstFail(cp, y),
                           [](const SearchStatistics& s) { return s.numberOfFailures() >= 3; }));
   auto stat = search.solve([](const SearchStatistics& s) { return s.numberOfNodes() >= 500; });
   CHECK(!stat.getCompleted());
   CHECK(nCapture == 0);
   CHECK(nFreeze == 0);
}

// Minimize the sum of 4 pairwise-different values in [0,9]; the core search
// tries the largest values first, so the first solutions are poor.
struct SumModel {
   CPSolver::Ptr    _cp;
   Veci             _x;
   var<int>::Ptr    _total;
   std::vector<int> _best;
   SumModel() : _cp(makeSolver()), _x(intVarArray(_cp, 4, 0, 9)), _best(4) {
      for (int i = 0; i < 4; i++)
         for (int j = i + 1; j < 4; j++)
            _cp->post(notEqual(_x[i], _x[j]));
      _total = Factory::sum(_x);
   }
   std::function<Branches(void)> largestFirst() {
      auto cp = _cp; auto x = _x;
      return forall(cp, range(0, 3), [cp, x](int i) { return label(cp, x[i], [](int v) { return -v; }); });
   }
   void capture() { for (int i = 0; i < 4; i++) _best[i] = _x[i]->min(); }
   void freezeAll() { for (int i = 0; i < 4; i++) _cp->post(_x[i] == _best[i]); }
};

static void testLnsStall()
{
   // freeze fixes the whole incumbent, so no iteration after the first can
   // improve: with maxStall = 2, lns runs exactly 2 more iterations and stops
   // by itself (the search completes, no outer limit needed).
   SumModel m;
   int nFreeze = 0, nCapture = 0;
   DFSearch search(m._cp, lns(m._cp,
      [&] { nCapture++; m.capture(); },
      [&] { nFreeze++; m.freezeAll(); },
      m.largestFirst(),
      [](const SearchStatistics& s) { return s.numberOfFailures() >= 20; },
      2));
   auto stat = search.optimize(minimize(m._total), [](const SearchStatistics& s) { return s.numberOfNodes() >= 100000; });
   CHECK(stat.getCompleted());
   CHECK(nCapture >= 1);
   CHECK(nFreeze == 2);
}

static void testLnsStallNeedsIncumbent()
{
   // Without a solution, iterations are not stalls: maxStall = 1 must not stop
   // the lns on an infeasible model; only the outer limit does.
   CPSolver::Ptr cp = makeSolver();
   auto y = pigeonhole(cp, 5);
   int nCapture = 0, nFreeze = 0;
   DFSearch search(cp, lns(cp, [&] { nCapture++; }, [&] { nFreeze++; }, firstFail(cp, y),
                           [](const SearchStatistics& s) { return s.numberOfFailures() >= 3; }, 1));
   auto stat = search.solve([](const SearchStatistics& s) { return s.numberOfNodes() >= 500; });
   CHECK(!stat.getCompleted());
   CHECK(nCapture == 0);
   CHECK(nFreeze == 0);
}

static void testAndThenOrder()
{
   // Plain branchings: andThen enumerates a's solutions, then b's, from the
   // same root state (b sees x unconstrained by a's choices).
   CPSolver::Ptr cp = makeSolver();
   auto x = intVarArray(cp, 1, 0, 2);
   bool inB = false;
   std::vector<std::pair<bool, int>> sols;
   auto a = [cp, x]() -> Branches { return label(cp, x[0]); };
   auto b = [cp, x, &inB]() -> Branches { inB = true; return label(cp, x[0], [](int v) { return -v; }); };
   DFSearch search(cp, andThen(cp, a, b));
   search.onSolution([&] { sols.push_back({inB, x[0]->min()}); });
   auto stat = search.solve();
   CHECK(stat.getCompleted());
   CHECK((sols == std::vector<std::pair<bool, int>>{{false, 0}, {false, 1}, {false, 2},
                                                    {true, 2}, {true, 1}, {true, 0}}));
}

static void testAndThenKeepsIncumbent()
{
   // Phase a: lns with a poor core, cut at 5 failures per iteration, stopped
   // after 2 stalls. Phase b: a complete smallest-first search from the root.
   // b only finds solutions better than a's best, and reaches the optimum 6.
   SumModel m;
   bool inB = false;
   std::vector<std::pair<bool, int>> sols;
   auto a = lns(m._cp, [&] { m.capture(); }, [&] { m.freezeAll(); }, m.largestFirst(),
                [](const SearchStatistics& s) { return s.numberOfFailures() >= 5; }, 2);
   auto cp = m._cp; auto x = m._x;
   auto smallest = forall(cp, range(0, 3), [cp, x](int i) { return label(cp, x[i]); });
   auto b = [&inB, smallest]() -> Branches { inB = true; return smallest(); };
   DFSearch search(m._cp, andThen(m._cp, a, b));
   search.onSolution([&] { sols.push_back({inB, m._total->min()}); });
   auto stat = search.optimize(minimize(m._total));
   CHECK(stat.getCompleted());
   int bestA = INT_MAX, nB = 0;
   for (auto& [phaseB, v] : sols) {
      if (!phaseB) bestA = std::min(bestA, v);
      else {
         nB++;
         CHECK(v < bestA);              // b starts from a's incumbent
      }
   }
   CHECK(bestA < INT_MAX);              // a found solutions
   CHECK(nB >= 1);                      // and b improved on them
   CHECK(sols.back().second == 6);      // 0 + 1 + 2 + 3
}

int main()
{
   testVanilla();
   testOrderedBy();
   testOrderedByPair();
   testSuchThat();
   testSuchThatEndsLoop();
   testLabel();
   testSequence();
   testLandVariadic();
   testSequenceResource();
   testLimitFailures();
   testLimitDoneScope();
   testRepeat();
   testRepeatLimitedIterations();
   testRepeatStopsOnScopedLimit();
   testRepeatStopsOnSearchLimit();
   testLns();
   testLnsFreezeNeedsSolution();
   testLnsStall();
   testLnsStallNeedsIncumbent();
   testAndThenOrder();
   testAndThenKeepsIncumbent();
   if (failures) {
      std::cerr << failures << " failure(s)\n";
      return 1;
   }
   std::cout << "testForall: all tests passed\n";
   return 0;
}
