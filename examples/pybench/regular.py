#!/usr/bin/env python3
# Port of examples/basic/regular.cpp: a regular constraint over 10 variables.
# The automaton (states 1..5, start 1, final 5) accepts exactly the words
# 1 2 3* 2 1, so with n = 10 the only solution is 1 2 3 3 3 3 3 3 2 1.
from minicpp import *

cp = makeSolver()
n = 10
q = intVarArray(cp, n, 1, 10)          # 10 variables with domains 1..10
tf = [(1, 1, 2), (2, 2, 3), (3, 2, 4), (3, 3, 3), (4, 1, 5)]   # (from, symbol, to)
a = automaton(cp, 1, 10, 1, 5, 1, {5}, tf)

# cp.post(q[1] != 2)   # that would make it infeasible.
cp.post(regular(q, a))

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
