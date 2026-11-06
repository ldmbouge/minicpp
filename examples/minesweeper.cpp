// Minesweeper: port of Objective-CP's Clo/minesweeper/main.m (Copyright (c)
// 2015 NICTA, Laurent Michel and Pascal Van Hentenryck, MPL 2.0), itself based
// on Hakan Kjellerstrand's MiniZinc model (http://www.hakank.org/minizinc).
// Background: Ian Stewart, http://www.claymath.org/Popular_Lectures/Minesweeper/
// and Richard Kaye, http://web.mat.bham.ac.uk/R.W.Kaye/minesw/minesw.htm
//
// Model: one 0/1 variable per cell (1 = mine). A cell showing a number k has
// no mine, and the 3x3 neighbourhood around it (clipped at the border, the
// cell itself included, as in objcp) holds exactly k mines.
// Search: first fail on all cells (objcp's default heuristic FF); stop at the
// first solution.
//
// Usage:
//   minesweeper

#include <iostream>
#include <iomanip>
#include <vector>
#include "solver.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"

int main(int argc, char* argv[])
{
   using namespace Factory;
   const int X = -1;                     // unknown cell
   const int r = 10, c = 10;
   const int game[r][c] = {
      {1,X,X,2,X,2,X,2,X,X},
      {X,3,2,X,X,X,4,X,X,1},
      {X,X,X,1,3,X,X,X,4,X},
      {3,X,1,X,X,X,3,X,X,X},
      {X,2,1,X,1,X,X,3,X,2},
      {X,3,X,2,X,X,2,X,1,X},
      {2,X,X,3,2,X,X,2,X,X},
      {X,3,X,X,X,3,2,X,X,3},
      {X,X,3,X,3,3,X,X,X,X},
      {X,2,X,2,X,X,X,2,2,X}};
   for (int i = 0; i < r; i++) {
      for (int j = 0; j < c; j++)
         std::cout << std::setw(2) << game[i][j] << " ";
      std::cout << "\n";
   }

   CPSolver::Ptr cp = makeSolver();
   auto mines = intVarMatrix(cp, r, c, 0, 1);
   for (int i = 0; i < r; i++)
      for (int j = 0; j < c; j++)
         if (game[i][j] >= 0) {
            std::vector<var<int>::Ptr> s;
            for (int a = -1; a <= 1; a++)
               for (int b = -1; b <= 1; b++)
                  if (i + a >= 0 && j + b >= 0 && i + a < r && j + b < c)
                     s.push_back(mines[i + a][j + b]);
            cp->post(sum(s, game[i][j]));
            cp->post(mines[i][j] == 0);
         }

   DFSearch search(cp, firstFail(cp, mines.flat()));
   search.onSolution([&] {
      std::cout << "\n";
      for (int i = 0; i < r; i++) {
         for (int j = 0; j < c; j++)
            std::cout << std::setw(2) << mines[i][j]->min() << " ";
         std::cout << "\n";
      }
      std::cout << std::endl;
   });
   auto stat = search.solve([](const SearchStatistics& s) { return s.numberOfSolutions() >= 1; });
   std::cout << stat << std::endl;
   return 0;
}
