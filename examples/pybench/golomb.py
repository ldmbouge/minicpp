#!/usr/bin/env python3
# Port of examples/basic/golomb.cpp: Golomb ruler.
#
# Place n marks m[1] = 0 < m[2] < ... < m[n] so that all pairwise distances
# are different, minimizing the ruler length m[n].
#
# The C++ search is forall(i in 1..n) label(m[i]): label m[1..n] in order,
# trying m[i] == min then m[i] != min. Since every mark before the current
# one is bound, this is the same as always branching on the first unbound mark.
#
# Usage: golomb.py [n >= 3 (10)]
import sys
from minicpp import *

n = int(sys.argv[1]) if len(sys.argv) > 1 else 10
if n < 3:
    sys.exit(f"usage: {sys.argv[0]} [n >= 3]")

cp = makeSolver()
# m[1..n]; slot 0 unused.
m = [None] + [makeIntVar(cp, 0, n * n) for j in range(1, n + 1)]
# d[i][j] for 1 <= i < j <= n; other cells stay None.
d = [[None] * (n + 1) for i in range(n + 1)]
ad = []
for i in range(1, n + 1):
    for j in range(i + 1, n + 1):
        d[i][j] = makeIntVar(cp, 0, n * n)
        cp.post(equal(m[j], d[i][j], m[i]))     # m[j] == d[i][j] + m[i]
        ad.append(d[i][j])
cp.post(m[1] == 0)
for j in range(1, n + 1):
    cp.post(m[j] >= j * (j - 1) // 2)
for i in range(1, n + 1):
    for j in range(i + 1, n + 1):
        cp.post(d[i][j] >= (j - i) * (j - i + 1) // 2)
for i in range(2, n + 1):
    cp.post(m[i - 1] <= m[i])
cp.post(m[2] <= d[n - 1][n])
cp.post(allDifferentAC(ad))

marks = m[1:]

def label():
    v = next((v for v in marks if not v.isBound), None)
    if v is None:
        return Branches()
    c = v.min
    return Branches(lambda: cp.post(v == c), lambda: cp.post(v != c))

search = DFSearch(cp, label)
search.onSolution(lambda: print(f"length = {m[n].min}\tmarks: " + " ".join(str(v.min) for v in marks)))
stat = search.optimize(minimize(m[n]))
print(stat)
