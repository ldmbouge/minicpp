#!/usr/bin/env python3
# Port of examples/basic/debruijn.cpp: de Bruijn sequences, base 2.
# Usage: debruijn.py [n in 1..20 (3)] [m (2^n)]
import sys
from minicpp import *

base = 2
n = int(sys.argv[1]) if len(sys.argv) > 1 else 3
if not 1 <= n <= 20:
    sys.exit(f"usage: {sys.argv[0]} [n in 1..20] [m]")
up = base ** n
m = int(sys.argv[2]) if len(sys.argv) > 2 else up
up -= 1
if not 1 <= m <= up + 1:
    sys.exit(f"m must be in 1..{up + 1}")
print(f"Params: n={n} m={m} base={base}")

cp = makeSolver()
# 1-based as in the C++ version: slot 0 unused.
x, code, binary = [None], [None], [None]
for i in range(1, m + 1):
    x.append(makeIntVar(cp, 0, up))
    code.append(makeIntVar(cp, 0, base - 1))
    binary.append([None] + [makeIntVar(cp, 0, base - 1) for j in range(1, n + 1)])
gcc = intVarArray(cp, base, 0, m)

xs = x[1:]
cp.post(allDifferent(xs))
for i in range(2, m + 1):
    cp.post(x[1] <= x[i])
for i in range(1, m + 1):
    terms = [-x[i]]
    w = 1
    for j in range(n, 0, -1):          # weight base^(n-j)
        terms.append(binary[i][j] * w)
        w *= base
    cp.post(sum(terms, 0))
for i in range(2, m + 1):
    for j in range(2, n + 1):
        cp.post(equal(binary[i - 1][j], binary[i][j - 1], 0))
for j in range(2, n + 1):
    cp.post(equal(binary[m][j], binary[1][j - 1], 0))
for i in range(1, m + 1):
    cp.post(equal(code[i], binary[i][1], 0))
for v in range(base):
    cp.post(sum([isEqual(code[j], v) for j in range(1, m + 1)], gcc[v]))

# All the model's variables, in declaration order.
allv = xs + [binary[i][j] for i in range(1, m + 1) for j in range(1, n + 1)] \
          + code[1:] + [gcc[v] for v in range(base)]

search = DFSearch(cp, firstFail(cp, allv))
stat = search.solve()
print("#sol:", stat.numberOfSolutions())
print(stat)
