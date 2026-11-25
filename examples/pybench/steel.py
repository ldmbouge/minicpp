#!/usr/bin/env python3
# Port of examples/basic/steel.cpp: steel mill slab design. Assign orders to
# slabs (at most two colours per slab) and minimize the total loss, i.e. the
# unused capacity of the smallest standard slab size that fits each load.
#
# Usage: steel.py [data file (data/steel/bench_19_0)]
import builtins, os, sys
from minicpp import *

here = os.path.dirname(os.path.abspath(__file__))
fn = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "../../data/steel/bench_19_0")
with open(fn) as f:
    data = iter(int(t) for t in f.read().split())
nCapa = next(data)
capa = [next(data) for i in range(nCapa)]
maxCapa = capa[-1]
loss = [0] * (maxCapa + 1)
capaIdx = 0
for i in range(maxCapa):
    loss[i] = capa[capaIdx] - i
    if loss[i] == 0:
        capaIdx += 1
loss[0] = 0
nCol = next(data)
nSlab = next(data)
nOrder = nSlab
w, c = [], []
for i in range(nSlab):
    w.append(next(data))
    c.append(next(data) - 1)

cp = makeSolver()
x = intVarArray(cp, nOrder, nSlab)
l = intVarArray(cp, nSlab, maxCapa + 1)

inSlab = [[isEqual(x[i], j) for i in range(nOrder)] for j in range(nSlab)]

for j in range(nSlab):
    # As in C++: boolVarArray creates nCol variables, which are then replaced.
    presence = boolVarArray(cp, nCol, True)
    for col in range(nCol):
        presence[col] = makeBoolVar(cp)
        inSlabWithColor = [inSlab[j][i] for i in range(nOrder) if c[i] == col]
        cp.post(isClause(presence[col], inSlabWithColor))
    cp.post(sum(presence) <= 2)
for j in range(nSlab):
    wj = [inSlab[j][i] * w[i] for i in range(nOrder)]
    cp.post(sum(wj, l[j]))
cp.post(sum(l, builtins.sum(w)))   # minicpp's sum shadows the built-in one
losses = [element(loss, l[j]) for j in range(nSlab)]
totLoss = sum(losses)
obj = minimize(totLoss)

def branch():
    xs = selectMin(x, lambda v: v.size > 1, lambda v: v.size)
    if xs is None:
        return Branches()
    maxUsed = -1
    for xi in x:
        if xi.isBound and xi.min > maxUsed:
            maxUsed = xi.min
    # One alternative per slab already used, plus one new slab (symmetry breaking).
    return Branches([(lambda i=i: cp.post(xs == i)) for i in range(maxUsed + 2)])

def show():
    colorsInSlab = [set() for j in range(nSlab)]
    for i in range(nOrder):
        colorsInSlab[x[i].min].add(c[i])
    for s in colorsInSlab:
        if len(s) > 2:
            print("THERE IS A BUG!")
    print("---Total loss =", totLoss.min)

search = DFSearch(cp, branch)
search.onSolution(show)
stat = search.optimize(obj)
print(stat)
