#!/usr/bin/env python3
# Port of examples/basic/ais.cpp: all-interval series.
#
# x is a permutation of 0..N-1 whose successive differences
# y[i] = |x[i+1] - x[i]| are all different (and non-zero). All solutions.
#
# Usage: ais.py [-nN (8)]   (same flag form as the C++ version, e.g. -n10)
import sys
from minicpp import *

N = int(sys.argv[1][2:]) if len(sys.argv) >= 2 and sys.argv[1].startswith("-n") else 8

cp = makeSolver()
x = intVarArray(cp, N, 0, N - 1)
y = intVarArray(cp, N - 1, 0, N - 1)
for i in range(N - 1):
    cp.post(y[i] != 0)

cp.post(allDifferentAC(list(x)))
cp.post(allDifferentAC(list(y)))
for i in range(N - 1):
    cp.post(equalAbsDiff(y[i], x[i + 1], x[i]))

def branch():
    xi = selectFirst(x, lambda v: v.size > 1)
    if xi is None:
        return Branches()
    return indomain_min(cp, xi)

search = DFSearch(cp, branch)
stat = search.solve()
print(stat)
