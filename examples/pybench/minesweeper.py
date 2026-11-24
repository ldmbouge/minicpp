#!/usr/bin/env python3
# Port of examples/basic/minesweeper.cpp.
from minicpp import *

X = -1  # unknown cell
game = [
    [1, X, X, 2, X, 2, X, 2, X, X],
    [X, 3, 2, X, X, X, 4, X, X, 1],
    [X, X, X, 1, 3, X, X, X, 4, X],
    [3, X, 1, X, X, X, 3, X, X, X],
    [X, 2, 1, X, 1, X, X, 3, X, 2],
    [X, 3, X, 2, X, X, 2, X, 1, X],
    [2, X, X, 3, 2, X, X, 2, X, X],
    [X, 3, X, X, X, 3, 2, X, X, 3],
    [X, X, 3, X, 3, 3, X, X, X, X],
    [X, 2, X, 2, X, X, X, 2, 2, X]]
r, c = len(game), len(game[0])
for row in game:
    print(" ".join(f"{v:2}" for v in row))

cp = makeSolver()
mines = [[makeIntVar(cp, 0, 1) for j in range(c)] for i in range(r)]
for i in range(r):
    for j in range(c):
        if game[i][j] >= 0:
            s = [mines[i + a][j + b]
                 for a in (-1, 0, 1) for b in (-1, 0, 1)
                 if 0 <= i + a < r and 0 <= j + b < c]
            cp.post(sum(s, game[i][j]))
            cp.post(mines[i][j] == 0)

def show():
    print()
    for i in range(r):
        print(" ".join(f"{mines[i][j].min:2}" for j in range(c)))
    print()

search = DFSearch(cp, firstFail(cp, [v for row in mines for v in row]))
search.onSolution(show)
stat = search.solve(lambda s: s.numberOfSolutions() >= 1)
print(stat)
