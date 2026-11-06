// Orthogonal latin squares: port of Objective-CP's Clo/latinSquare/main.m
// (Copyright (c) 2015 NICTA, Laurent Michel and Pascal Van Hentenryck, MPL 2.0).
//
// Find two latin squares x and y of order n that are orthogonal: the pairs
// (x[i][j], y[i][j]) are all different.
//
// Model:
//  - x, y in 1..n; z[i][j] in 0..n*n-1 encodes the pair:
//    z == (x-1)*n + y-1, and, as channels, x == m2[z], y == m1[z] with
//    m1[k] = 1 + k%n, m2[k] = 1 + k/n (element, domain consistent);
//  - alldifferent (AC) on every row and column of x and of y, and on all of z;
//  - row i of x <=lex row i-1 of y (i = 1..n-1), as in objcp.
// Search: objcp's hand-written loop: pick the unbound z with the smallest
// domain (first one on ties) and try its smallest value, else remove it,
// staying on that z until it is bound. Unlike firstFail, which selects again
// after each removal, the variable is sticky: the right branch records it in
// a trailed index that the next node reuses while it is unbound.
// Stops at the first solution.
//
// Usage:
//   latinSquare [n (5)]

#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <climits>
#include <memory>
#include <vector>
#include "solver.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"
#include "lex.hpp"

int main(int argc, char* argv[])
{
   using namespace Factory;
   const int n = argc > 1 ? atoi(argv[1]) : 5;
   if (n < 1) {
      std::cerr << "usage: " << argv[0] << " [n >= 1]\n";
      return 2;
   }
   CPSolver::Ptr cp = makeSolver();
   auto x = intVarMatrix(cp, n, n, 1, n);
   auto y = intVarMatrix(cp, n, n, 1, n);
   auto z = intVarMatrix(cp, n, n, 0, n * n - 1);
   std::vector<int> m1(n * n), m2(n * n);
   for (int k = 0; k < n * n; k++) {
      m1[k] = 1 + k % n;
      m2[k] = 1 + k / n;
   }
   for (int i = 0; i < n; i++)
      for (int j = 0; j < n; j++) {
         cp->post(element(m2, z[i][j], x[i][j]));   // x == m2[z], domain consistent
         cp->post(element(m1, z[i][j], y[i][j]));   // y == m1[z]
         cp->post(equal(z[i][j], x[i][j] * n + (-n), y[i][j] + (-1)));   // z == (x-1)*n + (y-1)
      }
   for (int i = 0; i < n; i++) {
      std::vector<var<int>::Ptr> xr, xc, yr, yc;
      for (int j = 0; j < n; j++) {
         xr.push_back(x[i][j]);
         xc.push_back(x[j][i]);
         yr.push_back(y[i][j]);
         yc.push_back(y[j][i]);
      }
      cp->post(allDifferentAC(xr));
      cp->post(allDifferentAC(xc));
      cp->post(allDifferentAC(yr));
      cp->post(allDifferentAC(yc));
   }
   cp->post(allDifferentAC(z.flat()));
   for (int i = 1; i < n; i++) {
      std::vector<var<int>::Ptr> xi, yp;
      for (int j = 0; j < n; j++) {
         xi.push_back(x[i][j]);
         yp.push_back(y[i - 1][j]);
      }
      cp->post(lexLeq(xi, yp));
   }

   auto show = [n](const char* name, auto& m) {
      std::cout << name << "=\n";
      for (int i = 0; i < n; i++) {
         std::cout << "\t|";
         for (int j = 0; j < n; j++)
            std::cout << std::setw(2) << m[i][j]->min() << " ";
         std::cout << "|\n";
      }
   };
   // objcp's search. _current is the z being labelled (-1: select one); it is
   // trailed, so backtracking past a right branch forgets it.
   struct State {
      trail<int> _current;
      State(Trailer::Ptr sm) : _current(sm, -1) {}
   };
   auto st = std::make_shared<State>(cp->getStateManager());
   auto av = z.flat();
   DFSearch search(cp, [cp, st, av]() -> Branches {
      int i = st->_current;
      if (i < 0 || av[i]->isBound()) {
         i = -1;
         int sd = INT_MAX;
         for (int j = 0; j < (int)av.size(); j++) {
            if (av[j]->isBound()) continue;
            if (av[j]->size() < sd) { sd = av[j]->size(); i = j; }
         }
         if (i < 0) return Branches({});
      }
      auto x = av[i];
      int const v = x->min();
      return [cp, x, v] { cp->post(x == v); }
           | [cp, st, x, v, i] { st->_current = i; cp->post(x != v); };
   });
   search.onSolution([&] { show("x", x); show("y", y); show("z", z); });
   auto stat = search.solve([](const SearchStatistics& s) { return s.numberOfSolutions() >= 1; });
   std::cout << stat << std::endl;
   return 0;
}
