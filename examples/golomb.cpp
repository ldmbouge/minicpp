// Golomb ruler: port of Objective-CP's Clo/golomb/main.m (Copyright (c) 2015
// NICTA, Laurent Michel and Pascal Van Hentenryck, MPL 2.0).
//
// Place n marks m[1] = 0 < m[2] < ... < m[n] so that all pairwise distances
// are different, minimizing the ruler length m[n].
//
// Model (indices from 1, as in objcp), domains 0..n*n:
//  - d[i][j] = m[j] - m[i] for i < j; objcp also fixes d[i][j] = 0 for
//    i >= j; those cells are unused, so they are not created here;
//  - alldifferent (AC) on all the d[i][j];
//  - m[1] == 0, m[i-1] <= m[i], m[j] >= j(j-1)/2;
//  - d[i][j] >= (j-i)(j-i+1)/2 (a ruler with j-i+1 marks);
//  - symmetry breaking: m[2] <= d[n-1][n].
// Search: label m[1..n] in order, smallest value first.
//
// Usage:
//   golomb [n (10)]

#include <iostream>
#include <cstdlib>
#include <vector>
#include "solver.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"
#include "range.hpp"

int main(int argc, char* argv[])
{
   using namespace Factory;
   const int n = argc > 1 ? atoi(argv[1]) : 10;
   if (n < 3) {
      std::cerr << "usage: " << argv[0] << " [n >= 3]\n";
      return 2;
   }
   CPSolver::Ptr cp = makeSolver();
   // m[1..n]; slot 0 unused.
   std::vector<var<int>::Ptr> m(n + 1);
   for (int j = 1; j <= n; j++)
      m[j] = makeIntVar(cp, 0, n * n);
   // d[i][j] for 1 <= i < j <= n; other cells stay null.
   std::vector<std::vector<var<int>::Ptr>> d(n + 1, std::vector<var<int>::Ptr>(n + 1));
   std::vector<var<int>::Ptr> ad;
   for (int i = 1; i <= n; i++)
      for (int j = i + 1; j <= n; j++) {
         d[i][j] = makeIntVar(cp, 0, n * n);
         cp->post(equal(m[j], d[i][j], m[i]));      // m[j] == d[i][j] + m[i]
         ad.push_back(d[i][j]);
      }
   cp->post(m[1] == 0);
   for (int j = 1; j <= n; j++)
      cp->post(m[j] >= j * (j - 1) / 2);
   for (int i = 1; i <= n; i++)
      for (int j = i + 1; j <= n; j++)
         cp->post(d[i][j] >= (j - i) * (j - i + 1) / 2);
   for (int i = 2; i <= n; i++)
      cp->post(m[i - 1] <= m[i]);
   cp->post(m[2] <= d[n - 1][n]);
   cp->post(allDifferentAC(ad));

   DFSearch search(cp, forall(cp, range(1, n), [cp, &m](int i) { return label(cp, m[i]); }));
   search.onSolution([&m, n] {
      std::cout << "length = " << m[n]->min() << "\tmarks:";
      for (int i = 1; i <= n; i++)
         std::cout << " " << m[i]->min();
      std::cout << std::endl;
   });
   auto stat = search.optimize(minimize(m[n]));
   std::cout << stat << std::endl;
   return 0;
}
