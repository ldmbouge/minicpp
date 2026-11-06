// Slow convergence: port of Objective-CP's Clo/slow_convergence/main.m
// (Copyright (c) 2015 NICTA, Laurent Michel and Pascal Van Hentenryck, MPL 2.0).
//
// A propagation stress test: a chain of difference constraints whose bounds
// shrink by small steps, so the fixpoint takes many rounds to reach.
//
// Model: x[0..n], y[0..n] in 0..10n, and
//    y[i-1] <= y[i]              (i = 2..n)
//    y[0] - y[i] <= n - i + 1    (i = 1..n)
//    y[n] <= x[0]
//    x[i] <= x[j]                (1 <= i < j <= n)
//    y[0] >= n
// Search: label the variables in declaration order (y, then x), smallest
// value first, as objcp's labelArray over [model intVars]; stop at the first
// solution.
//
// Usage:
//   slow_convergence [n (100)]

#include <iostream>
#include <cstdlib>
#include <vector>
#include "solver.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"
#include "range.hpp"
#include "RuntimeMonitor.hpp"

int main(int argc, char* argv[])
{
   using namespace Factory;
   const int n = argc > 1 ? atoi(argv[1]) : 100;
   if (n < 1) {
      std::cerr << "usage: " << argv[0] << " [n >= 1]\n";
      return 2;
   }
   auto start = RuntimeMonitor::now();
   CPSolver::Ptr cp = makeSolver();
   auto y = intVarArray(cp, n + 1, 0, 10 * n);
   auto x = intVarArray(cp, n + 1, 0, 10 * n);
   for (int i = 2; i <= n; i++)
      cp->post(y[i - 1] <= y[i]);
   for (int i = 1; i <= n; i++)
      cp->post(y[0] <= y[i] + (n - i + 1));
   cp->post(y[n] <= x[0]);
   for (int i = 1; i <= n - 1; i++)
      for (int j = i + 1; j <= n; j++)
         cp->post(x[i] <= x[j]);
   cp->post(y[0] >= n);

   std::vector<var<int>::Ptr> vars(y.begin(), y.end());
   vars.insert(vars.end(), x.begin(), x.end());
   DFSearch search(cp, forall(cp, range(0, (int)vars.size() - 1),
                              [cp, &vars](int k) { return label(cp, vars[k]); }));
   search.onSolution([&] {
      std::cout << "x = " << x << "\ny = " << y << std::endl;
   });
   auto stat = search.solve([](const SearchStatistics& s) { return s.numberOfSolutions() >= 1; });
   std::cout << "Execution Time(WC): " << RuntimeMonitor::elapsedSince(start) << " ms" << std::endl;
   std::cout << stat << std::endl;
   return 0;
}
