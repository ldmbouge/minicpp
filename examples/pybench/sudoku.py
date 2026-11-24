#!/usr/bin/env python3
# Port of examples/basic/sudoku.cpp.
# Usage: sudoku.py [clue file (data/sudoku/sudokuFile3.txt)]
import os
import sys
from minicpp import *

here = os.path.dirname(os.path.abspath(__file__))
fn = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "../../data/sudoku/sudokuFile3.txt")
with open(fn) as f:
    tokens = [int(t) for t in f.read().split()]
nb = tokens[0]
clues = [tokens[1 + 3 * k: 4 + 3 * k] for k in range(nb)]
print("number of entries", nb)

cp = makeSolver()
x = [[makeIntVar(cp, 1, 9) for j in range(9)] for i in range(9)]
for r, c, v in clues:
    cp.post(x[r - 1][c - 1] == v)
for i in range(9):
    cp.post(allDifferent([x[i][j] for j in range(9)]))
    cp.post(allDifferent([x[j][i] for j in range(9)]))
for bi in range(3):
    for bj in range(3):
        cp.post(allDifferent([x[r][c] for r in range(3 * bi, 3 * bi + 3)
                                      for c in range(3 * bj, 3 * bj + 3)]))

def show():
    for i in range(9):
        print("  ".join(str(x[i][j].min) for j in range(9)))
    print()

search = DFSearch(cp, firstFail(cp, [v for row in x for v in row]))
search.onSolution(show)
stat = search.solve(lambda s: s.numberOfSolutions() >= 1)
print(stat)
