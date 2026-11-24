#!/usr/bin/env python3
# Port of examples/basic/costas.cpp: Costas arrays.
#
# A Costas array of order n is a permutation costas[1..n] whose difference
# vectors are all distinct: for every distance i, the differences
# costas[j] - costas[j-i] (j = i+1..n) are pairwise different.
# Search: all solutions, first fail on costas.
#
# Usage: costas.py [n >= 2 (8)]
import sys
from minicpp import *

n = int(sys.argv[1]) if len(sys.argv) > 1 else 8
if n < 2:
    sys.exit(f"usage: {sys.argv[0]} [n >= 2]")

cp = makeSolver()
# costas[1..n]; slot 0 unused.
costas = [None] + [makeIntVar(cp, 1, n) for j in range(1, n + 1)]
cp.post(allDifferentAC(costas[1:]))

# diff[i][j] for 1 <= i < j <= n; other cells stay None.
diff = [[None] * (n + 1) for i in range(n + 1)]
for i in range(1, n + 1):
    for j in range(i + 1, n + 1):
        diff[i][j] = makeIntVar(cp, -n + 1, n - 1)
        cp.post(equal(costas[j], diff[i][j], costas[j - i]))   # costas[j] == diff + costas[j-i]
        cp.post(diff[i][j] != 0)
for i in range(1, n):
    cp.post(allDifferent(diff[i][i + 1:]))
cp.post(costas[1] <= costas[n])
for k in range(3, n + 1):
    for l in range(k + 1, n + 1):
        cp.post(sum([diff[k - 2][l - 1], diff[k][l], -diff[k - 1][l - 1], -diff[k - 1][l]], 0))

search = DFSearch(cp, firstFail(cp, costas[1:]))
stat = search.solve()
print("#sols:", stat.numberOfSolutions())
print(stat)
