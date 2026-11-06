// Tests for the failure-directed search branching in fds.hpp.
//
// FDS only changes the order in which binary choices are explored, so the
// tests check it against plain labelling on the same models:
//  - completeness: the same set of solutions, each found once, including
//    domains wider than the 30 initial splits (the addMoreChoices path) and
//    negative domains;
//  - an infeasible model is proved infeasible;
//  - a model whose variables are already fixed gives one solution, no choice;
//  - the search is deterministic and leaves the domains as it found them;
//  - with land(fds, label(makespan)), optimize reaches ft06's optimum (55).

#include <iostream>
#include <fstream>
#include <filesystem>
#include <set>
#include <vector>
#include "solver.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"
#include "range.hpp"
#include "fds.hpp"
#include "jobshop.hpp"

using namespace Factory;

static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::cerr << "CHECK failed line " << __LINE__ << ": " #c "\n"; failures++; } } while (0)

typedef std::vector<std::vector<int>> Solutions;

// Plain labelling: variables in order, smallest value first.
static std::function<Branches(void)> labelAll(CPSolver::Ptr cp, const std::vector<var<int>::Ptr>& x)
{
   return forall(cp, range(0, (int)x.size() - 1), [cp, x](int i) { return label(cp, x[i]); });
}

// Every solution of the model built by make(cp), in the order found, using
// the branching built by branching(cp, x).
template <class Make, class Branching>
static Solutions solveAll(Make make, Branching branching, SearchStatistics* stats = nullptr)
{
   CPSolver::Ptr cp = makeSolver();
   std::vector<var<int>::Ptr> x = make(cp);
   DFSearch search(cp, branching(cp, x));
   Solutions sols;
   search.onSolution([&] {
      std::vector<int> s;
      for (auto& v : x) s.push_back(v->min());
      sols.push_back(s);
   });
   auto st = search.solve();
   if (stats) *stats = st;
   return sols;
}

static auto withFds = [](CPSolver::Ptr cp, const std::vector<var<int>::Ptr>& x) { return fds(cp, x); };
static auto withLabel = [](CPSolver::Ptr cp, const std::vector<var<int>::Ptr>& x) { return labelAll(cp, x); };

// FDS must find exactly the solutions labelling finds, each once.
template <class Make>
static void sameSolutions(Make make, size_t expected)
{
   Solutions a = solveAll(make, withLabel);
   Solutions b = solveAll(make, withFds);
   std::set<std::vector<int>> sa(a.begin(), a.end()), sb(b.begin(), b.end());
   CHECK(a.size() == expected);
   CHECK(b.size() == expected);
   CHECK(sb.size() == b.size());          // no duplicates
   CHECK(sa == sb);
}

static void testSmallModel()
{
   // x < y, y + z == 6, all different, in 0..5
   sameSolutions([](CPSolver::Ptr cp) {
      auto x = intVarArray(cp, 3, 0, 5);
      cp->post(x[0] < x[1]);
      cp->post(sum(std::vector<var<int>::Ptr>{x[1], x[2]}, 6));
      cp->post(allDifferent(x));
      return std::vector<var<int>::Ptr>(x.begin(), x.end());
   }, 10);
}

static void testWideDomain()
{
   // 0..200: the 30 breadth-first initial splits leave intervals of about
   // 6 values unresolved, so FDS must add choices to fix the variable.
   sameSolutions([](CPSolver::Ptr cp) {
      auto x = intVarArray(cp, 1, 0, 200);
      return std::vector<var<int>::Ptr>(x.begin(), x.end());
   }, 201);
}

static void testWideDomainsWithPropagation()
{
   // x + y == 1000, x >= 990, both in 0..1000: 11 solutions.
   sameSolutions([](CPSolver::Ptr cp) {
      auto x = intVarArray(cp, 2, 0, 1000);
      cp->post(sum(std::vector<var<int>::Ptr>{x[0], x[1]}, 1000));
      cp->post(x[0] >= 990);
      return std::vector<var<int>::Ptr>(x.begin(), x.end());
   }, 11);
}

static void testNegativeDomains()
{
   // x, y in -6..6, x + y == 0, x != y: 12 solutions.
   sameSolutions([](CPSolver::Ptr cp) {
      auto x = intVarArray(cp, 2, -6, 6);
      cp->post(sum(std::vector<var<int>::Ptr>{x[0], x[1]}, 0));
      cp->post(x[0] != x[1]);
      return std::vector<var<int>::Ptr>(x.begin(), x.end());
   }, 12);
}

static void testInfeasible()
{
   // 5 pigeons, 4 holes.
   SearchStatistics st;
   Solutions s = solveAll([](CPSolver::Ptr cp) {
      auto x = intVarArray(cp, 5, 0, 3);
      cp->post(allDifferent(x));
      return std::vector<var<int>::Ptr>(x.begin(), x.end());
   }, withFds, &st);
   CHECK(s.empty());
   CHECK(st.numberOfFailures() > 0);
}

static void testAllFixed()
{
   SearchStatistics st;
   Solutions s = solveAll([](CPSolver::Ptr cp) {
      auto x = intVarArray(cp, 3, 2, 2);
      return std::vector<var<int>::Ptr>(x.begin(), x.end());
   }, withFds, &st);
   CHECK(s.size() == 1);
   CHECK(st.numberOfNodes() == 0);
}

static void testDeterministic()
{
   auto make = [](CPSolver::Ptr cp) {
      auto x = intVarArray(cp, 6, 0, 7);
      cp->post(allDifferent(x));
      cp->post(sum(std::vector<var<int>::Ptr>{x[0], x[1], x[2]}, 12));
      return std::vector<var<int>::Ptr>(x.begin(), x.end());
   };
   SearchStatistics s1, s2;
   Solutions a = solveAll(make, withFds, &s1);
   Solutions b = solveAll(make, withFds, &s2);
   CHECK(a == b);                                       // same solutions, same order
   CHECK(s1.numberOfNodes() == s2.numberOfNodes());
   CHECK(s1.numberOfFailures() == s2.numberOfFailures());
}

static void testStateRestored()
{
   CPSolver::Ptr cp = makeSolver();
   auto x = intVarArray(cp, 4, 0, 9);
   cp->post(allDifferent(x));
   cp->post(x[0] < x[3]);
   std::vector<var<int>::Ptr> v(x.begin(), x.end());
   std::vector<std::pair<int, int>> before;
   std::vector<int> sizeBefore;
   for (auto& xi : v) {
      before.push_back({xi->min(), xi->max()});
      sizeBefore.push_back(xi->size());
   }
   DFSearch search(cp, fds(cp, v));
   auto st = search.solve();
   CHECK(st.numberOfSolutions() == 10 * 9 * 8 * 7 / 2);
   for (size_t i = 0; i < v.size(); i++) {
      CHECK(v[i]->min() == before[i].first);
      CHECK(v[i]->max() == before[i].second);
      CHECK(v[i]->size() == sizeBefore[i]);
   }
}

static void testJobShopOptimum()
{
   // ft06 (Fisher and Thompson 6x6), optimum 55.
   auto path = std::filesystem::temp_directory_path() / "testFDS_ft06.jss";
   {
      std::ofstream f(path);
      f << "6 6\n"
           "2 1 0 3 1 6 3 7 5 3 4 6\n"
           "1 8 2 5 4 10 5 10 0 10 3 4\n"
           "2 5 3 4 5 8 0 9 1 1 4 7\n"
           "1 5 0 5 2 5 3 3 4 8 5 9\n"
           "2 9 1 3 4 5 5 4 0 3 3 1\n"
           "1 3 3 3 5 9 0 10 4 4 2 1\n";
   }
   Instance in(path.string());
   std::filesystem::remove(path);
   CHECK(in.valid());
   if (!in.valid()) return;

   CPSolver::Ptr cp = makeSolver();
   JobShopModel model(cp, in);
   auto makespan = model.makespan();
   std::vector<var<int>::Ptr> starts;
   for (int i = 0; i < in.nbJobs(); i++)
      for (int j = 0; j < in.nbMachines(); j++)
         starts.push_back(model.start(i, j));
   DFSearch search(cp, land(fds(cp, starts), [cp, makespan] { return label(cp, makespan); }));
   int best = INT_MAX;
   bool valid = true;
   search.onSolution([&] {
      valid = valid && model.checkSchedule();
      best = makespan->min();
   });
   auto st = search.optimize(Factory::minimize(makespan));
   CHECK(valid);
   CHECK(best == 55);
   CHECK(st.getCompleted());
}

int main()
{
   testSmallModel();
   testWideDomain();
   testWideDomainsWithPropagation();
   testNegativeDomains();
   testInfeasible();
   testAllFixed();
   testDeterministic();
   testStateRestored();
   testJobShopOptimum();
   if (failures) {
      std::cerr << failures << " failure(s)\n";
      return 1;
   }
   std::cout << "testFDS: all tests passed\n";
   return 0;
}
