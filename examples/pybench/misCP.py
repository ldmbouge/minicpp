#!/usr/bin/env python3
# Port of examples/tutorial/misCP.cpp: maximum weighted independent set.
from minicpp import *

w = [5, 4, 2, 6, 8]
edges = [(0, 1), (0, 4), (1, 2), (1, 3), (2, 3), (3, 4)]

cp = makeSolver()
x = intVarArray(cp, 5, 0, 1)
z = makeIntVar(cp, 0, 10000)

cp.post(sum(x, w) == z)
for a, b in edges:
    cp.post(sum([x[a], x[b]]) <= 1)

obj = maximize(z)
print("VARS:", x, "\tZ=", z)

def branch():
    xi = selectFirst(x, lambda v: v.size > 1)
    if xi is None:
        return Branches()
    return indomain_max(cp, xi)

search = DFSearch(cp, branch)
search.onSolution(lambda: print("Assignment:", x, "\t OBJ:", z))
stat = search.optimize(obj)
print(stat)
