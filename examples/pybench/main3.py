#!/usr/bin/env python3
# Port of examples/basic/main3.cpp: 8-queens (binary != constraints) whose search
# combines two first-fail branchings with land: the first half of the queens is
# labelled before the second half. All solutions.
from minicpp import *

n = 8
cp = makeSolver()
q = intVarArray(cp, n, 1, n)
for i in range(n):
    for j in range(i + 1, n):
        cp.post(q[i] != q[j])
        cp.post(notEqual(q[i], q[j], i - j))
        cp.post(notEqual(q[i], q[j], j - i))

q1 = [q[i] for i in range(n // 2)]
q2 = [q[n // 2 + i] for i in range(n // 2)]

search = DFSearch(cp, land([firstFail(cp, q1), firstFail(cp, q2)]))

nbSol = 0
def onSol():
    global nbSol
    print("sol =", q)
    nbSol += 1
search.onSolution(onSol)

stat = search.solve()
print(stat)
print("Got:", nbSol, "solutions")
