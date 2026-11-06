// Costas arrays: port of Objective-CP's Clo/costas/main.m (Copyright (c) 2015
// NICTA, Laurent Michel and Pascal Van Hentenryck, MPL 2.0).
//
// A Costas array of order n is a permutation costas[1..n] whose difference
// vectors are all distinct: for every distance i, the differences
// costas[j] - costas[j-i] (j = i+1..n) are pairwise different.
//
// Model (indices from 1, as in objcp):
//  - alldifferent (AC) on costas;
//  - diff[i][j] = costas[j] - costas[j-i] for i < j, with diff != 0;
//    objcp also fixes diff[i][j] = 0 for i >= j; those cells are unused, so
//    they are not created here;
//  - alldifferent on each row diff[i][i+1..n];
//  - symmetry breaking: costas[1] <= costas[n];
//  - the redundant constraints
//    diff[k-2][l-1] + diff[k][l] == diff[k-1][l-1] + diff[k-1][l].
// Search: all solutions, first fail on costas (objcp's default heuristic FF).
//
// Usage:
//   costas [n (8)]

#include <iostream>
#include <cstdlib>
#include <vector>
#include "solver.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"

int main(int argc, char* argv[])
{
   using namespace Factory;
   const int n = argc > 1 ? atoi(argv[1]) : 8;
   if (n < 2) {
      std::cerr << "usage: " << argv[0] << " [n >= 2]\n";
      return 2;
   }
   CPSolver::Ptr cp = makeSolver();
   // costas[1..n]; slot 0 unused.
   std::vector<var<int>::Ptr> costas(n + 1);
   for (int j = 1; j <= n; j++)
      costas[j] = makeIntVar(cp, 1, n);
   cp->post(allDifferentAC(std::vector<var<int>::Ptr>(costas.begin() + 1, costas.end())));

   // diff[i][j] for 1 <= i < j <= n; other cells stay null.
   std::vector<std::vector<var<int>::Ptr>> diff(n + 1, std::vector<var<int>::Ptr>(n + 1));
   for (int i = 1; i <= n; i++)
      for (int j = i + 1; j <= n; j++) {
         diff[i][j] = makeIntVar(cp, -n + 1, n - 1);
         cp->post(equal(costas[j], diff[i][j], costas[j - i]));   // costas[j] == diff + costas[j-i]
         cp->post(diff[i][j] != 0);
      }
   for (int i = 1; i <= n - 1; i++)
      cp->post(allDifferent(std::vector<var<int>::Ptr>(diff[i].begin() + i + 1, diff[i].end())));
   cp->post(costas[1] <= costas[n]);
   for (int k = 3; k <= n; k++)
      for (int l = k + 1; l <= n; l++)
         cp->post(sum(std::vector<var<int>::Ptr>{diff[k - 2][l - 1], diff[k][l],
                                                  -diff[k - 1][l - 1], -diff[k - 1][l]}, 0));

   std::vector<var<int>::Ptr> vars(costas.begin() + 1, costas.end());
   DFSearch search(cp, firstFail(cp, vars));
   auto stat = search.solve();
   std::cout << "#sols: " << stat.numberOfSolutions() << std::endl;
   std::cout << stat << std::endl;
   return 0;
}
