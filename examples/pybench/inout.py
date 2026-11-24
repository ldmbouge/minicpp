#!/usr/bin/env python3
# Port of examples/basic/inout.cpp: n-queens (three AC all-different) whose
# search branches with inside(x, {c}) / outside(x, {c}) instead of x == c / x != c.
from minicpp import *

n = 12
cp = makeSolver()
q = intVarArray(cp, n, 1, n)
cp.post(allDifferentAC([q[i] for i in range(n)]))
cp.post(allDifferentAC([q[i] + i for i in range(n)]))
cp.post(allDifferentAC([q[i] - i for i in range(n)]))

def branch():
    x = selectMin(q, lambda x: x.size > 1, lambda x: x.size)
    if x is None:
        return Branches()
    CS = {x.min}
    return Branches(lambda: cp.post(inside(x, CS)), lambda: cp.post(outside(x, CS)))

search = DFSearch(cp, branch)
stat = search.solve()
print(stat)
