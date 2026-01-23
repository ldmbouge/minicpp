#!/usr/local/bin/python3
from minicpp import *
n=10
cp = makeSolver()
x  = intVarArray(cp,n,n)

cp.post(allDifferent(x))

print("Starting search...")
search = DFSearch(cp,firstFail(cp,x))
search.onSolution(lambda : print("objective = %d " % obj.value()))
stat = search.optimize(obj)
print(stat)


