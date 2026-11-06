// Nonogram: port of Objective-CP's Clo/nonogram/main.m (Copyright (c) 2015
// NICTA, Laurent Michel and Pascal Van Hentenryck, MPL 2.0). The instance is
// Gecode's P200 (http://www.gecode.org/gecode-doc-latest/classNonogram.html).
//
// Model: one 0/1 variable per cell (1 = filled). Each row and each column must
// match its clue, a sequence of block lengths; the clue becomes an automaton
// (make_transition_matrix below) posted with regular.
// Search: all solutions. objcp labels row by row when
// rows*rowRuleLen < cols*colRuleLen and column by column otherwise
// (column by column for P200), trying 1 before 0.
//
// Usage:
//   nonogram

#include <iostream>
#include <set>
#include <vector>
#include <cassert>
#include "solver.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"
#include "regular.hpp"

// objcp's make_transition_matrix: the automaton accepting the 0/1 strings
// whose blocks of 1s have the lengths in pattern, in order. States 0..nbStates-1
// follow the template w b^p1 w b^p2 w ... w: a white state loops on w and
// moves to the next state on b; a black state moves on the next symbol of the
// template. The last two states are final.
// objcp's handling of an empty clue reads past its template; an empty clue
// is handled here by a single white state (all cells empty). P200 has none.
static Automaton::Ptr clueAutomaton(CPSolver::Ptr cp, const std::vector<int>& pattern)
{
   const int w = 0, b = 1;
   if (pattern.empty())
      return Factory::automaton(cp, 0, 1, 0, 0, 0, {0}, {{0, w, 0}});
   const int pLen = (int)pattern.size();
   int nbStates = pLen;
   for (int p : pattern) nbStates += p;
   nbStates += 1;
   std::vector<int> tmp;
   tmp.push_back(w);
   for (int p : pattern) {
      for (int j = 0; j < p; j++) tmp.push_back(b);
      tmp.push_back(w);
   }
   assert((int)tmp.size() == nbStates);
   std::vector<Transition> t;
   for (int k = 0; k < nbStates; k++) {
      if (tmp[k] == w) {
         t.push_back({k, w, k});
         if (k < nbStates - 1)
            t.push_back({k, b, k + 1});
      } else
         t.push_back({k, tmp[k + 1] == b ? b : w, k + 1});
   }
   assert((int)t.size() == (nbStates - 1 - pLen) + 2 * (pLen + 1) - 1);   // objcp: bs + 2*ws - 1
   return Factory::automaton(cp, 0, 1, 0, nbStates - 1, 0, {nbStates - 1, nbStates - 2}, t);
}

// objcp's checkRule: drop the 0 padding of a clue row and post regular.
template <class Vec>
static void checkRule(CPSolver::Ptr cp, const int* rules, int len, const Vec& y)
{
   std::vector<int> pattern;
   for (int k = 0; k < len; k++)
      if (rules[k] > 0)
         pattern.push_back(rules[k]);
   cp->post(Factory::regular(y, clueAutomaton(cp, pattern)));
}

int main(int argc, char* argv[])
{
   using namespace Factory;
   const int rows = 25, rowRuleLen = 7;
   const int rowRules[rows][rowRuleLen] = {
      {0,0,0,0,2,2,3},
      {0,0,4,1,1,1,4},
      {0,0,4,1,2,1,1},
      {4,1,1,1,1,1,1},
      {0,2,1,1,2,3,5},
      {0,1,1,1,1,2,1},
      {0,0,3,1,5,1,2},
      {0,3,2,2,1,2,2},
      {2,1,4,1,1,1,1},
      {0,2,2,1,2,1,2},
      {0,1,1,1,3,2,3},
      {0,0,1,1,2,7,3},
      {0,0,1,2,2,1,5},
      {0,0,3,2,2,1,2},
      {0,0,0,3,2,1,2},
      {0,0,0,0,5,1,2},
      {0,0,0,2,2,1,2},
      {0,0,0,4,2,1,2},
      {0,0,0,6,2,3,2},
      {0,0,0,7,4,3,2},
      {0,0,0,0,7,4,4},
      {0,0,0,0,7,1,4},
      {0,0,0,0,6,1,4},
      {0,0,0,0,4,2,2},
      {0,0,0,0,0,2,1}};
   const int cols = 25, colRuleLen = 6;
   const int colRules[cols][colRuleLen] = {
      {0,0,1,1,2,2},
      {0,0,0,5,5,7},
      {0,0,5,2,2,9},
      {0,0,3,2,3,9},
      {0,1,1,3,2,7},
      {0,0,0,3,1,5},
      {0,7,1,1,1,3},
      {1,2,1,1,2,1},
      {0,0,0,4,2,4},
      {0,0,1,2,2,2},
      {0,0,0,4,6,2},
      {0,0,1,2,2,1},
      {0,0,3,3,2,1},
      {0,0,0,4,1,15},
      {1,1,1,3,1,1},
      {2,1,1,2,2,3},
      {0,0,1,4,4,1},
      {0,0,1,4,3,2},
      {0,0,1,1,2,2},
      {0,7,2,3,1,1},
      {0,2,1,1,1,5},
      {0,0,0,1,2,5},
      {0,0,1,1,1,3},
      {0,0,0,4,2,1},
      {0,0,0,0,0,3}};

   CPSolver::Ptr cp = makeSolver();
   auto x = intVarMatrix(cp, rows, cols, 0, 1);
   for (int i = 0; i < rows; i++) {
      std::vector<var<int>::Ptr> row;
      for (int k = 0; k < cols; k++) row.push_back(x[i][k]);
      checkRule(cp, rowRules[i], rowRuleLen, row);
   }
   for (int i = 0; i < cols; i++) {
      std::vector<var<int>::Ptr> col;
      for (int k = 0; k < rows; k++) col.push_back(x[k][i]);
      checkRule(cp, colRules[i], colRuleLen, col);
   }

   // Cells in objcp's labelling order.
   std::vector<var<int>::Ptr> order;
   if (rows * rowRuleLen < cols * colRuleLen) {
      for (int r = 0; r < rows; r++)
         for (int c = 0; c < cols; c++) order.push_back(x[r][c]);
   } else {
      for (int c = 0; c < cols; c++)
         for (int r = 0; r < rows; r++) order.push_back(x[r][c]);
   }
   DFSearch search(cp, [cp, order]() -> Branches {
      for (auto& v : order)
         if (!v->isBound())
            return [cp, v] { cp->post(v == 1); }
                 | [cp, v] { cp->post(v == 0); };
      return Branches({});
   });
   search.onSolution([&] {
      for (int i = 0; i < rows; i++) {
         std::cout << "|";
         for (int j = 0; j < cols; j++)
            std::cout << (x[i][j]->min() == 0 ? ' ' : '#');
         std::cout << "|\n";
      }
   });
   auto stat = search.solve();
   std::cout << stat << std::endl;
   return 0;
}
