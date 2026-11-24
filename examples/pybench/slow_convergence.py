#!/usr/bin/env python3
# Port of examples/basic/slow_convergence.cpp: a propagation stress test.
# Usage: slow_convergence.py [n >= 1 (100)]
#
# The C++ search is forall(k in vars) label(vars[k]): label the variables in
# declaration order (y, then x), trying x == min(x) then x != min(x). Since
# every variable before the current one is bound, this is the same as always
# branching on the first unbound variable.
import sys
import time
from minicpp import *

n = int(sys.argv[1]) if len(sys.argv) > 1 else 100
if n < 1:
    sys.exit(f"usage: {sys.argv[0]} [n >= 1]")
start = time.perf_counter()
cp = makeSolver()
y = intVarArray(cp, n + 1, 0, 10 * n)
x = intVarArray(cp, n + 1, 0, 10 * n)
for i in range(2, n + 1):
    cp.post(y[i - 1] <= y[i])
for i in range(1, n + 1):
    cp.post(y[0] <= y[i] + (n - i + 1))
cp.post(y[n] <= x[0])
for i in range(1, n):
    for j in range(i + 1, n + 1):
        cp.post(x[i] <= x[j])
cp.post(y[0] >= n)

allv = [y[i] for i in range(n + 1)] + [x[i] for i in range(n + 1)]

def label():
    v = next((v for v in allv if not v.isBound), None)
    if v is None:
        return Branches()
    c = v.min
    return Branches(lambda: cp.post(v == c), lambda: cp.post(v != c))

search = DFSearch(cp, label)
search.onSolution(lambda: print("x =", x, "\ny =", y))
stat = search.solve(lambda s: s.numberOfSolutions() >= 1)
print(f"Execution Time(WC): {int((time.perf_counter() - start) * 1000)} ms")
print(stat)
