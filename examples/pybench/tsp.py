#!/usr/local/bin/python3
from minicpp import *

with open("examples/data/tsp.txt") as f:
    n = int(f.readline())
    data = [[int(v) for v in line.split()] for line in f.readlines()]
        
print(data)

cp = makeSolver()
succ = intVarArray(cp,n,n)
distSucc = intVarArray(cp,n,1000)

cp.post(circuit(succ))
for i in range(0,n):
    cp.post(element(data[i],succ[i],distSucc[i]))
obj = minimize(sum(distSucc))

print("Starting search...")
search = DFSearch(cp,firstFail(cp,succ))
search.onSolution(lambda : print("objective = %d " % obj.value()))
stat = search.optimize(obj)
print(stat)


