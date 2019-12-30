#!/usr/local/bin/python3
import minicpp

n  = 8
cp = minicpp.makeSolver()
q  = minicpp.intVarArray(cp,n,1,n)

for i in range(0,n):
   for j in range(i+1,n):
       cp.post(q[i] != q[j])
       cp.post(q[i] != q[j] + i - j)
       cp.post(q[i] != q[j] + j - i)

print("Starting search...")
search = minicpp.DFSearch(cp,minicpp.firstFail(cp,q))
search.onSolution(lambda : print(q))
stat = search.solve()
print(stat)
