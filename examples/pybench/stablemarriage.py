#!/usr/bin/env python3
# Port of examples/basic/stablemarriage.cpp: stable marriage as a CP model.
# All stable matchings.
#
# Usage: stablemarriage.py [data file (data/stablemarriage/stable_marriage.txt)]
import os, sys
from minicpp import *

here = os.path.dirname(os.path.abspath(__file__))
fn = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "../../data/stablemarriage/stable_marriage.txt")
# Like the C++ reader, take only the 1 + 2n^2 integers needed (the file has a
# trailing non-numeric line).
with open(fn) as f:
    tokens = f.read().split()
n = int(tokens[0])
data = [int(t) for t in tokens[:1 + 2 * n * n]]
rankWomen = [data[1 + i * n: 1 + (i + 1) * n] for i in range(n)]
rankMen = [data[1 + n * n + i * n: 1 + n * n + (i + 1) * n] for i in range(n)]

cp = makeSolver()
wife = intVarArray(cp, n, n)
husband = intVarArray(cp, n, n)
wifePref = intVarArray(cp, n, n + 1)
husbandPref = intVarArray(cp, n, n + 1)

# C++ element(rankWomen[m], ...) indexes a matrix row, which selects the
# bounds-consistent Element1D: that is elementBC here.
for m in range(n):
    cp.post(husband[wife[m]] == m)
    cp.post(elementBC(rankWomen[m], wife[m], wifePref[m]))
for w in range(n):
    cp.post(wife[husband[w]] == w)
    cp.post(elementBC(rankMen[w], husband[w], husbandPref[w]))
for m in range(n):
    for w in range(n):
        mPrefersW = isLarger(wifePref[m], rankWomen[m][w])
        wDont = isLess(husbandPref[w], rankMen[w][m])
        cp.post(implies(mPrefersW, wDont) == True)

        wPrefersM = isLarger(husbandPref[w], rankMen[w][m])
        mDont = isLess(wifePref[m], rankWomen[m][w])
        cp.post(implies(wPrefersM, mDont) == True)

search = DFSearch(cp, land([firstFail(cp, wife), firstFail(cp, husband)]))

def show():
    print("wife = " + "".join("%3d " % v.min for v in wife)
          + " | husband = " + "".join("%3d " % v.min for v in husband))
search.onSolution(show)

stat = search.solve()
print(stat)
