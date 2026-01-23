#!/usr/local/bin/python3
from minicpp import *

# with open("../examples/data/qap.txt") as f:
#     n = int(f.readline())
#     f.readline() # skip blank line
#     data = [[int(v) for v in line.split()] for line in f.readlines()]
#     w = data[:n]
#     d = data[n+1:]

# print(w)
# print(d)
n=12
print("n=",n)

cp = makeSolver()
x  = intVarArray(cp,n,n)

print(x)

cp.post(allDifferent(x))
wDist = [w[i][j] * element(d,x[i],x[j]) for i in range(0,n) for j in range(0,n)]

print(type(wDist))
print(wDist)

obj = minimize(sum(wDist))

print("Starting search...")
search = DFSearch(cp,firstFail(cp,x))
search.onSolution(lambda : print("objective = %d " % obj.value()))
stat = search.optimize(obj)
print(stat)


