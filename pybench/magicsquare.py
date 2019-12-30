#!/usr/local/bin/python3
from minicpp import *
n  = 6
sumResult =  n * (n*n + 1)//2

cp = makeSolver()
x  = [[makeIntVar(cp,1,n**2) for j in range(0,n)] for i in range(0,n)]
fx = [var for cols in x for var in cols]

cp.post(allDifferent(fx))
for i in range(0,n):
    cp.post(sum([x[i][j] for j in range(0,n)],sumResult))
for j in range(0,n):
    cp.post(sum([x[i][j] for i in range(0,n)],sumResult))
cp.post(sum([x[i][i]     for i in range(0,n)],sumResult))
cp.post(sum([x[n-i-1][i] for i in range(0,n)],sumResult))

print("Starting search...")
search = DFSearch(cp,firstFail(cp,fx))
search.onSolution(lambda : print(*x,sep = "\n"))
stat = search.solve(lambda s : s.numberOfSolutions() > 0)
print(stat)


