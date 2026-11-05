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
   if (failures) {
      std::cerr << failures << " failure(s)\n";
      return 1;
   }
   std::cout << "testForall: all tests passed\n";
   return 0;
}
