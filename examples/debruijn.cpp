// de Bruijn sequences: port of Objective-CP's Clo/debruijn/main.m
// (Copyright (c) 2015 NICTA, Laurent Michel and Pascal Van Hentenryck, MPL 2.0).
//
// Find m distinct n-digit numbers in base 2, x[1..m], arranged in a cycle in
// which each number is the previous one shifted left by one digit with a new
// digit appended (so their first digits spell a de Bruijn sequence when
// m = 2^n).
//
// Model (indices from 1, as in objcp):
//  - x[i] in 0..2^n-1, all different (objcp: value consistency);
//  - symmetry breaking: x[1] <= x[i];
//  - x[i] == sum_j binary[i][j] * 2^(n-j), binary[i][j] in 0..1;
//  - binary[i-1][j] == binary[i][j-1] (the shift), wrapping from m to 1;
//  - code[i] == binary[i][1];
//  - gcc[v] == number of i with code[i] == v (counts, not constrained further).
// Search: all solutions; first fail on every variable (objcp's default
// heuristic over the whole model), then x in order (nothing left by then).
//
// Usage:
//   debruijn [n (3)] [m (2^n)]

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
   const int base = 2;
   const int n = argc > 1 ? atoi(argv[1]) : 3;
   if (n < 1 || n > 20) {
      std::cerr << "usage: " << argv[0] << " [n in 1..20] [m]\n";
      return 2;
   }
   int up = 1;
   for (int k = 0; k < n; k++) up *= base;
   const int m = argc > 2 ? atoi(argv[2]) : up;
   up -= 1;
   if (m < 1 || m > up + 1) {
      std::cerr << "m must be in 1.." << up + 1 << "\n";
      return 2;
   }
   std::cout << "Params: n=" << n << " m=" << m << " base=" << base << std::endl;

   CPSolver::Ptr cp = makeSolver();
   // 1-based: slot 0 unused.
   std::vector<var<int>::Ptr> x(m + 1), code(m + 1);
   std::vector<std::vector<var<int>::Ptr>> binary(m + 1, std::vector<var<int>::Ptr>(n + 1));
   for (int i = 1; i <= m; i++) {
      x[i] = makeIntVar(cp, 0, up);
      code[i] = makeIntVar(cp, 0, base - 1);
      for (int j = 1; j <= n; j++)
         binary[i][j] = makeIntVar(cp, 0, base - 1);
   }
   auto gcc = intVarArray(cp, base, 0, m);

   std::vector<var<int>::Ptr> xs(x.begin() + 1, x.end());
   cp->post(allDifferent(xs));
   for (int i = 2; i <= m; i++)
      cp->post(x[1] <= x[i]);
   for (int i = 1; i <= m; i++) {
      std::vector<var<int>::Ptr> terms{-x[i]};
      int w = 1;
      for (int j = n; j >= 1; j--, w *= base)        // weight base^(n-j)
         terms.push_back(binary[i][j] * w);
      cp->post(sum(terms, 0));
   }
   for (int i = 2; i <= m; i++)
      for (int j = 2; j <= n; j++)
         cp->post(equal(binary[i - 1][j], binary[i][j - 1]));
   for (int j = 2; j <= n; j++)
      cp->post(equal(binary[m][j], binary[1][j - 1]));
   for (int i = 1; i <= m; i++)
      cp->post(equal(code[i], binary[i][1]));
   for (int v = 0; v < base; v++) {
      std::vector<var<bool>::Ptr> is;
      for (int j = 1; j <= m; j++)
         is.push_back(isEqual(code[j], v));
      cp->post(sum(is, gcc[v]));
   }

   // All the model's variables, in declaration order.
   std::vector<var<int>::Ptr> all(xs);
   for (int i = 1; i <= m; i++)
      for (int j = 1; j <= n; j++)
         all.push_back(binary[i][j]);
   all.insert(all.end(), code.begin() + 1, code.end());
   all.insert(all.end(), gcc.begin(), gcc.end());

   DFSearch search(cp, firstFail(cp, all));
   auto stat = search.solve();
   std::cout << "#sol: " << stat.numberOfSolutions() << std::endl;
   std::cout << stat << std::endl;
   return 0;
}
