#!/usr/bin/python3
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
def doIt():
    sx = minicpp.selectMin(q,lambda x : x.size > 1,lambda x : x.size)
    return minicpp.indomain_min(cp,sx)
        
search = minicpp.DFSearch(cp,doIt)
search.onSolution(lambda : print(q))
stat = search.solve(lambda s : s.numberOfSolutions() > 100000)
print(stat)
