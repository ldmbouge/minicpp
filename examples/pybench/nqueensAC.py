#!/usr/bin/env python3
# Port of examples/basic/nqueensAC.cpp: n-queens with three AC all-different.
from minicpp import *

n = 8
cp = makeSolver()
q = intVarArray(cp, n, 1, n)
cp.post(allDifferentAC([q[i] for i in range(n)]))
cp.post(allDifferentAC([q[i] + i for i in range(n)]))
cp.post(allDifferentAC([q[i] - i for i in range(n)]))

def branch():
    x = selectMin(q, lambda x: x.size > 1, lambda x: x.size)
    if x is None:
        return Branches()
    c = x.min
    return Branches(lambda: cp.post(x == c), lambda: cp.post(x != c))

search = DFSearch(cp, branch)
stat = search.solve()
print(stat)
