// Knight's tour (Euler): port of Objective-CP's Clo/Euler/main.m
// (Copyright (c) 2015 NICTA, Laurent Michel and Pascal Van Hentenryck, MPL 2.0).
//
// Find a closed knight's tour of the 8x8 board. Squares are numbered 1..64
// row by row, as in objcp; internally jump[k] is 0-based (square k+1), since
// minicpp's circuit works on 0..n-1.
//
// Model: jump[i] (the square after i) is restricted to the knight moves from
// i, alldifferent (AC), and circuit.
// Search: first fail on jump, smallest value first; stop at the first
// solution and print the tour.
//
// Usage:
//   euler

#include <iostream>
#include <set>
#include <vector>
#include "solver.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"

// objcp's knightMoves, on 1-based squares: the targets of a knight on square
// i (column i % 8, with 0 for the last column). Targets off the board (< 1 or
// > 64) are dropped.
static std::set<int> knightMoves(int i)
{
   std::vector<int> d;
   switch (i % 8) {
      case 1:  d = {-15, -6, 10, 17}; break;
      case 2:  d = {-17, -15, -6, 10, 15, 17}; break;
      case 7:  d = {-17, -15, -10, 6, 15, 17}; break;
      case 0:  d = {-17, -10, 6, 15}; break;
      default: d = {-17, -15, -10, -6, 6, 10, 15, 17}; break;
   }
   std::set<int> s;
   for (int k : d)
      if (i + k >= 1 && i + k <= 64)
         s.insert(i + k);
   return s;
}

int main(int argc, char* argv[])
{
   using namespace Factory;
   CPSolver::Ptr cp = makeSolver();
   auto jump = intVarArray(cp, 64, 0, 63);     // jump[k] is square k+1's successor, 0-based
   for (int i = 1; i <= 64; i++) {
      std::set<int> s;
      for (int t : knightMoves(i))
         s.insert(t - 1);
      cp->post(inside(jump[i - 1], s));
   }
   cp->post(allDifferentAC(jump));
   cp->post(circuit(jump));

   DFSearch search(cp, firstFail(cp, jump));
   search.onSolution([&jump] {
      int curr = 0;
      std::cout << "1";
      do {
         curr = jump[curr]->min();
         std::cout << "->" << curr + 1;
      } while (curr != 0);
      std::cout << std::endl;
   });
   auto stat = search.solve([](const SearchStatistics& s) { return s.numberOfSolutions() >= 1; });
   std::cout << stat << std::endl;
   return 0;
}
