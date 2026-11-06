// Sudoku: port of Objective-CP's Clo/Sudoku/main.m (Copyright (c) 2015 NICTA,
// Laurent Michel and Pascal Van Hentenryck, MPL 2.0).
//
// Model: a 9x9 matrix with domain 1..9, the clues from the file, and an
// alldifferent on every row, column and 3x3 block.
// Search: label the cells by smallest domain, values in increasing order;
// stop at the first solution.
//
// File format: the number of clues, then one "row col value" triple per clue,
// rows and columns numbered from 1.
//
// Usage:
//   sudoku [file (data/sudoku/sudokuFile3.txt)]

#include <iostream>
#include <fstream>
#include <vector>
#include "solver.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"

int main(int argc, char* argv[])
{
   using namespace Factory;
   const char* fn = argc > 1 ? argv[1] : "data/sudoku/sudokuFile3.txt";
   std::ifstream f(fn);
   int nb;
   if (!(f >> nb)) {
      std::cerr << "cannot read " << fn << "\n";
      return 1;
   }
   std::cout << "number of entries " << nb << std::endl;

   CPSolver::Ptr cp = makeSolver();
   auto x = intVarMatrix(cp, 9, 9, 1, 9);
   for (int k = 0; k < nb; k++) {
      int r, c, v;
      if (!(f >> r >> c >> v) || r < 1 || r > 9 || c < 1 || c > 9 || v < 1 || v > 9) {
         std::cerr << "bad clue " << k + 1 << " in " << fn << "\n";
         return 1;
      }
      cp->post(x[r - 1][c - 1] == v);
   }
   for (int i = 0; i < 9; i++) {
      std::vector<var<int>::Ptr> row, col;
      for (int j = 0; j < 9; j++) {
         row.push_back(x[i][j]);
         col.push_back(x[j][i]);
      }
      cp->post(allDifferent(row));
      cp->post(allDifferent(col));
   }
   for (int bi = 0; bi < 3; bi++)
      for (int bj = 0; bj < 3; bj++) {
         std::vector<var<int>::Ptr> block;
         for (int r = 3 * bi; r < 3 * bi + 3; r++)
            for (int c = 3 * bj; c < 3 * bj + 3; c++)
               block.push_back(x[r][c]);
         cp->post(allDifferent(block));
      }

   DFSearch search(cp, firstFail(cp, x.flat()));
   search.onSolution([&x] {
      for (int i = 0; i < 9; i++) {
         for (int j = 0; j < 9; j++)
            std::cout << x[i][j]->min() << "  ";
         std::cout << "\n";
      }
      std::cout << std::endl;
   });
   auto stat = search.solve([](const SearchStatistics& s) { return s.numberOfSolutions() >= 1; });
   std::cout << stat << std::endl;
   return 0;
}
