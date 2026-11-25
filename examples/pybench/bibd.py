#!/usr/bin/env python3
# Port of examples/basic/bibd.cpp: balanced incomplete block design (v, k, l).
#
# A v x b 0/1 matrix where every row has r ones, every column has k ones and
# every pair of rows shares exactly l ones. Rows and columns are ordered with
# lexLeq to break symmetries. Stops at the first solution.
#
# Usage: bibd.py [-nA (1)]   A indexes the instance table below (as in C++).
import sys
from minicpp import *

instances = [
    (7, 3, 1), (6, 3, 2), (8, 4, 3), (7, 3, 20), (7, 3, 30),
    (7, 3, 40), (7, 3, 45), (7, 3, 50), (7, 3, 55), (7, 3, 60),
    (7, 3, 300), (8, 4, 5), (8, 4, 6), (8, 4, 7),
]
a = int(sys.argv[1][2:]) if len(sys.argv) >= 2 and sys.argv[1].startswith("-n") else 1
v, k, l = instances[a]
b = (v * (v - 1) * l) // (k * (k - 1))
r = l * (v - 1) // (k - 1)

cp = makeSolver()
M = [[makeBoolVar(cp) for j in range(b)] for i in range(v)]

for i in range(v):
    cp.post(sum([M[i][j] for j in range(b)], r))
for j in range(b):
    cp.post(sum([M[i][j] for i in range(v)], k))
for i in range(v):
    for j in range(i + 1, v):
        cp.post(sum([M[i][x] * M[j][x] for x in range(b)], l))
for i in range(v - 1):
    cp.post(lexLeq([M[i + 1][j] for j in range(b)], [M[i][j] for j in range(b)]))
for j in range(b - 1):
    cp.post(lexLeq([M[i][j + 1] for i in range(v)], [M[i][j] for i in range(v)]))

def show():
    for row in M:
        print(" ".join("1" if x.isTrue() else "0" for x in row) + " ")

search = DFSearch(cp, firstFail(cp, [x for row in M for x in row]))
search.onSolution(show)
stat = search.solve(lambda s: s.numberOfSolutions() >= 1)
print(stat)
