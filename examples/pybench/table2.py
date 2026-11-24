#!/usr/bin/env python3
# Port of examples/basic/table2.cpp: a table constraint that, with the extra
# constraints below, is infeasible before search starts.
from minicpp import *

n = 3
cp = makeSolver()
x = intVarArray(cp, n, 1, 50)
cp.post(x[0] <= 10)
cp.post(x[1] >= 30)
cp.post(x[2] >= 5)
rows = [[1, 30, 8], [2, 32, 9], [4, 32, 9], [2, 31, 9], [4, 36, 9], [5, 36, 9]]

try:
    cp.post(x[1] >= 36)
    cp.post(table(x, rows))
    cp.post(x[0] != 5)
    cp.post(x[0] != 4)

    def branch():
        xi = selectMin(x, lambda v: v.size > 1, lambda v: v.size)
        if xi is None:
            return Branches()
        c = xi.min
        return Branches(lambda: cp.post(xi == c), lambda: cp.post(xi != c))

    search = DFSearch(cp, branch)
    search.onSolution(lambda: print("sol =", x))
    stat = search.solve()
    print(stat)
except RuntimeError:   # the solver's failure surfaces as RuntimeError("FAIL")
    print("Something off infeasible from the start...")
