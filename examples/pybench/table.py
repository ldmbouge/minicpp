#!/usr/bin/env python3
# Port of examples/basic/table.cpp: a small table constraint, all solutions.
from minicpp import *

n = 3
cp = makeSolver()
q = intVarArray(cp, n, 1, 4)
cp.post(table(q, [[1, 2, 3], [3, 2, 1], [2, 1, 4]]))
cp.post(q[1] != 2)

def branch():
    x = selectMin(q, lambda x: x.size > 1, lambda x: x.size)
    if x is None:
        return Branches()
    c = x.min
    return Branches(lambda: cp.post(x == c), lambda: cp.post(x != c))

search = DFSearch(cp, branch)
search.onSolution(lambda: print("sol =", q))
stat = search.solve()
print(stat)
