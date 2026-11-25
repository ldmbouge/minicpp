#!/usr/bin/env python3
# Port of examples/basic/latinSquare.cpp: two orthogonal latin squares x, y of
# order n (the pairs (x[i][j], y[i][j]) are all different).
#
# z[i][j] == (x-1)*n + (y-1) encodes the pair, channelled with element.
# Search (objcp's): pick the unbound z with the smallest domain and try its
# smallest value, else remove it and stay on that z until it is bound.
# Stops at the first solution.
#
# Usage: latinSquare.py [n (5)]
import sys
from minicpp import *

n = int(sys.argv[1]) if len(sys.argv) > 1 else 5
if n < 1:
    sys.exit("usage: latinSquare.py [n >= 1]")

cp = makeSolver()
x = [intVarArray(cp, n, 1, n) for i in range(n)]
y = [intVarArray(cp, n, 1, n) for i in range(n)]
z = [intVarArray(cp, n, 0, n * n - 1) for i in range(n)]
m1 = [1 + k % n for k in range(n * n)]
m2 = [1 + k // n for k in range(n * n)]
for i in range(n):
    for j in range(n):
        cp.post(element(m2, z[i][j], x[i][j]))      # x == m2[z], domain consistent
        cp.post(element(m1, z[i][j], y[i][j]))      # y == m1[z]
        cp.post(equal(z[i][j], x[i][j] * n + (-n), y[i][j] + (-1)))  # z == (x-1)*n + (y-1)
for i in range(n):
    cp.post(allDifferentAC([x[i][j] for j in range(n)]))
    cp.post(allDifferentAC([x[j][i] for j in range(n)]))
    cp.post(allDifferentAC([y[i][j] for j in range(n)]))
    cp.post(allDifferentAC([y[j][i] for j in range(n)]))
av = [v for row in z for v in row]
cp.post(allDifferentAC(av))
for i in range(1, n):
    cp.post(lexLeq([x[i][j] for j in range(n)], [y[i - 1][j] for j in range(n)]))

def show(name, m):
    print(name + "=")
    for row in m:
        print("\t|" + "".join("%2d " % v.min for v in row) + "|")

# The C++ version keeps the z being labelled in a trail<int>. A plain variable
# set only by the right branch holds the same value: in depth-first search the
# right branch executed last is always an ancestor of the current node, since
# leaving its subtree means backtracking to a pending right branch that runs next.
current = -1

def branch():
    global current
    i = current
    if i < 0 or av[i].isBound:
        i, sd = -1, None
        for j, v in enumerate(av):
            if not v.isBound and (sd is None or v.size < sd):
                sd, i = v.size, j
        if i < 0:
            return Branches()
    xi = av[i]
    c = xi.min
    def right():
        global current
        current = i
        cp.post(xi != c)
    return Branches(lambda: cp.post(xi == c), right)

search = DFSearch(cp, branch)
search.onSolution(lambda: (show("x", x), show("y", y), show("z", z)))
stat = search.solve(lambda s: s.numberOfSolutions() >= 1)
print(stat)
