#!/usr/bin/env python3
# Port of examples/basic/euler.cpp: closed knight's tour of the 8x8 board.
#
# Squares are numbered 1..64 row by row; internally jump[k] is 0-based
# (square k+1), since circuit works on 0..n-1.
# Model: jump[i] restricted to the knight moves from i, alldifferent (AC), circuit.
# Search: first fail on jump; stop at the first solution and print the tour.
from minicpp import *

def knightMoves(i):
    """The targets of a knight on 1-based square i (column i % 8, 0 for the last)."""
    d = {1: [-15, -6, 10, 17],
         2: [-17, -15, -6, 10, 15, 17],
         7: [-17, -15, -10, 6, 15, 17],
         0: [-17, -10, 6, 15]}.get(i % 8, [-17, -15, -10, -6, 6, 10, 15, 17])
    return {i + k for k in d if 1 <= i + k <= 64}

cp = makeSolver()
jump = intVarArray(cp, 64, 0, 63)    # jump[k] is square k+1's successor, 0-based
for i in range(1, 65):
    cp.post(inside(jump[i - 1], {t - 1 for t in knightMoves(i)}))
cp.post(allDifferentAC(list(jump)))
cp.post(circuit(jump))

def show():
    tour, curr = ["1"], 0
    while True:
        curr = jump[curr].min
        tour.append(str(curr + 1))
        if curr == 0:
            break
    print("->".join(tour))

search = DFSearch(cp, firstFail(cp, jump))
search.onSolution(show)
stat = search.solve(lambda s: s.numberOfSolutions() >= 1)
print(stat)
