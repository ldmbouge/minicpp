#!/usr/local/bin/python3
from minicpp import *
n=3
cp = makeSolver()
x  = intVarArray(cp,n,n)
print(type(x))
    

y = allDifferent(x)
print(y)
cp.post(y)

def sol(t):
    return [t[i].min for i in range(0,len(t))]

sol(x)

print("Starting search...")
search = DFSearch(cp,firstFail(cp,x))
search.onSolution(lambda : print("solution = ",sol(x)))
stat = search.solve()
print(stat)


