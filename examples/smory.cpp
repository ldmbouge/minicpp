// SEND + MORE = MONEY: port of Objective-CP's Clo/smory/main.m
// (Copyright (c) 2015 NICTA, Laurent Michel and Pascal Van Hentenryck, MPL 2.0).
//
// Model: one digit (0..9) per letter, all different, S and M non-zero, and
//    SEND + MORE == MONEY
// as one linear equation with integer coefficients (MONEY moved to the left
// with negated coefficients). As in objcp, the letters that occur more than
// once (E, N, M, O) are not merged into a single term.
// Search: all solutions, first fail on the letters (objcp's default heuristic
// FF, followed by a first-fail labelArray that has nothing left to do).
//
// Usage:
//   smory

#include <iostream>
#include <vector>
#include "solver.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"

int main(int argc, char* argv[])
{
   using namespace Factory;
   CPSolver::Ptr cp = makeSolver();
   auto S = makeIntVar(cp, 0, 9), E = makeIntVar(cp, 0, 9), N = makeIntVar(cp, 0, 9),
        D = makeIntVar(cp, 0, 9), M = makeIntVar(cp, 0, 9), O = makeIntVar(cp, 0, 9),
        R = makeIntVar(cp, 0, 9), Y = makeIntVar(cp, 0, 9);
   std::vector<var<int>::Ptr> x{S, E, N, D, M, O, R, Y};
   const char* names = "SENDMORY";
   cp->post(allDifferent(x));
   cp->post(M != 0);
   cp->post(S != 0);

   std::vector<var<int>::Ptr> send{S, E, N, D}, more{M, O, R, E}, money{M, O, N, E, Y};
   std::vector<int> c1{1000, 100, 10, 1}, c2{10000, 1000, 100, 10, 1};
   std::vector<var<int>::Ptr> terms;
   for (int i = 0; i < 4; i++) terms.push_back(send[i] * c1[i]);
   for (int i = 0; i < 4; i++) terms.push_back(more[i] * c1[i]);
   for (int i = 0; i < 5; i++) terms.push_back(money[i] * -c2[i]);
   cp->post(sum(terms, 0));

   DFSearch search(cp, firstFail(cp, x));
   search.onSolution([&] {
      std::cout << "Sol:";
      for (size_t i = 0; i < x.size(); i++)
         std::cout << " " << names[i] << "=" << x[i]->min();
      std::cout << std::endl;
   });
   auto stat = search.solve();
   std::cout << stat << std::endl;
   return 0;
}
