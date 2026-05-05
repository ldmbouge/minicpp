#!/usr/bin/python3
import minicpp
#from python import minicpp

n  = 12
cp = minicpp.makeSolver()
q  = minicpp.intVarArray(cp,n,1,n)

for i in range(0,n):
   for j in range(i+1,n):
       cp.post(q[i] != q[j])
       cp.post(q[i] != q[j] + i - j)
       cp.post(q[i] != q[j] + j - i)

print("Starting search...")

def doIt():
    sx = minicpp.selectMin(q,lambda x : x.size > 1,lambda x : x.size)
    if sx is not None:
        c = sx.min
        return minicpp.Branches(lambda : cp.post(sx == c),
                                lambda : cp.post(sx != c))
    else:
        return minicpp.Branches()
        
search = minicpp.DFSearch(cp,doIt)
search.onSolution(lambda : print([q[i].min for i in range(0,n)]))
#stat = search.solve(lambda s : s.numberOfSolutions() > 1000)
stat = search.solve()
print(stat)
