#!/usr/local/bin/python3
from minicpp import *

n  = 20
cp = makeSolver()
s  = intVarArray(cp,n,n)

for i in range(0,n):
    cp.post(sum([isEqual(s[j],i) for j in range(0,n)],s[i]))
cp.post(sum(s,n))

print("Starting search...")
search = DFSearch(cp,firstFail(cp,s))
search.onSolution(lambda : print(s))
stat = search.solve()
print(stat)
