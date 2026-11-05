// Tests for the Disjunctive (unary resource) constraint.
// The oracle is brute-force enumeration of start times, so it is independent
// of any other propagator in the library.

#include <iostream>
#include <random>
#include <cstdlib>
#include "solver.hpp"
#include "intvar.hpp"
#include "search.hpp"
#include "fail.hpp"
#include "scheduling.hpp"

using namespace Factory;

static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::cerr << "CHECK failed line " << __LINE__ << ": " #c "\n"; failures++; } } while (0)

static bool noOverlap(const std::vector<int>& s, const std::vector<int>& d)
{
   for (auto i = 0u; i < s.size(); i++)
      for (auto j = i + 1; j < s.size(); j++)
         if (d[i] > 0 && d[j] > 0 && s[i] + d[i] > s[j] && s[j] + d[j] > s[i])
            return false;
   return true;
}

// Number of assignments in [lo[i],hi[i]] that satisfy the disjunctive.
static int bruteCount(const std::vector<int>& lo, const std::vector<int>& hi,
                      const std::vector<int>& d)
{
   std::vector<int> s(lo);
   int count = 0;
   for (;;) {
      if (noOverlap(s, d)) count++;
      int k = 0;
      while (k < (int)s.size() && s[k] == hi[k]) { s[k] = lo[k]; k++; }
      if (k == (int)s.size()) break;
      s[k]++;
   }
   return count;
}

// Solve with the propagator; also check every reported solution.
static int solveCount(const std::vector<int>& lo, const std::vector<int>& hi,
                      const std::vector<int>& d, Disjunctive::Options opt)
{
   CPSolver::Ptr cp = makeSolver();
   auto s = intVarArray(cp, (int)lo.size());
   for (auto i = 0u; i < lo.size(); i++)
      s[i] = makeIntVar(cp, lo[i], hi[i]);
   bool ok = true;
   TRYFAIL
      cp->post(disjunctive(s, d, opt));
   ONFAIL
      ok = false;
   ENDFAIL
   if (!ok) return 0;
   DFSearch search(cp, firstFail(cp, s));
   int bad = 0;
   search.onSolution([&]() {
      std::vector<int> v;
      for (auto& x : s) v.push_back(x->min());
      if (!noOverlap(v, d)) bad++;
   });
   auto stat = search.solve();
   CHECK(bad == 0);
   return stat.numberOfSolutions();
}

static void testAllDiff()
{
   // Unit durations: equivalent to alldifferent over 5 values.
   std::vector<int> lo(5, 0), hi(5, 4), d(5, 1);
   CHECK(solveCount(lo, hi, d, {}) == 120);
}

static void testDetectablePrecedence()
{
   CPSolver::Ptr cp = makeSolver();
   Veci s = intVarArray(cp, 2);
   s[0] = makeIntVar(cp, 0, 10);   // d = 10: ect 10, lst 10
   s[1] = makeIntVar(cp, 6, 15);   // d = 6:  ect 12 > lst(s0) = 10, so s0 precedes s1
   std::vector<int> d{10, 6};
   cp->post(disjunctive(s, d));
   CHECK(s[1]->min() == 10);
}

static void testOverload()
{
   CPSolver::Ptr cp = makeSolver();
   Veci s = intVarArray(cp, 3);
   s[0] = makeIntVar(cp, 0, 9);
   s[1] = makeIntVar(cp, 1, 10);
   s[2] = makeIntVar(cp, 3, 7);
   std::vector<int> d{5, 5, 6};   // 16 units of work in the window [0,15)
   bool failed = false;
   TRYFAIL
      cp->post(disjunctive(s, d));
   ONFAIL
      failed = true;
   ENDFAIL
   CHECK(failed);
}

static void testEdgeFinding()
{
   // Classic example (Vilim): A and B must both complete before C can start.
   // A: est 0, lct 11, p 4;  B: est 1, lct 10, p 3;  C: est 1, lct 18, p 5.
   // {A,B} ∪ {C} cannot finish by lct(A,B) = 11  -> est(C) >= ECT({A,B}) = 7.
   CPSolver::Ptr cp = makeSolver();
   Veci s = intVarArray(cp, 3);
   s[0] = makeIntVar(cp, 0, 7);
   s[1] = makeIntVar(cp, 1, 7);
   s[2] = makeIntVar(cp, 1, 13);
   std::vector<int> d{4, 3, 5};
   Disjunctive::Options efOnly{false, false, true};
   cp->post(disjunctive(s, d, efOnly));
   CHECK(s[2]->min() == 7);
}

static void testZeroDuration()
{
   // A zero-duration task is ignored by the resource.
   std::vector<int> lo{0, 0, 0}, hi{3, 3, 3}, d{2, 0, 2};
   CHECK(solveCount(lo, hi, d, {}) == bruteCount(lo, hi, d));
}

static void testRandom(int rounds)
{
   std::mt19937 rng(42);
   Disjunctive::Options configs[] = {
      {true, true, true}, {true, false, false}, {false, true, false}, {false, false, true}
   };
   for (int r = 0; r < rounds; r++) {
      int n = 2 + rng() % 5;                 // 2..6 tasks
      std::vector<int> lo(n), hi(n), d(n);
      for (int i = 0; i < n; i++) {
         d[i]  = 1 + rng() % 5;
         lo[i] = rng() % 8;
         hi[i] = lo[i] + rng() % (n <= 4 ? 10 : 6);
      }
      int expected = bruteCount(lo, hi, d);
      for (auto& opt : configs) {
         int got = solveCount(lo, hi, d, opt);
         if (got != expected) {
            std::cerr << "random #" << r << " opt{" << opt.dprec << opt.nfnl << opt.ef
                      << "}: expected " << expected << " got " << got << "\n  d =";
            for (int x : d) std::cerr << ' ' << x;
            std::cerr << "\n  dom =";
            for (int i = 0; i < n; i++) std::cerr << " [" << lo[i] << ',' << hi[i] << ']';
            std::cerr << '\n';
            failures++;
         }
      }
   }
}

int main(int argc, char* argv[])
{
   int rounds = argc > 1 ? atoi(argv[1]) : 300;
   testAllDiff();
   testDetectablePrecedence();
   testOverload();
   testEdgeFinding();
   testZeroDuration();
   testRandom(rounds);
   if (failures) {
      std::cerr << failures << " failure(s)\n";
      return 1;
   }
   std::cout << "testDisj: all tests passed\n";
   return 0;
}
