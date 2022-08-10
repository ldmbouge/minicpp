import minicpp

n = 8
cp = minicpp.makeSolver()
print(cp)

a0 = minicpp.makeIntVar(cp,1,n)
q = minicpp.intVarArray(cp,n,1,n)

print(a0)
print(q)

qi = q[1]
print(qi)
print(len(q))

print("PRIOR Loop")
for x in q:
    print(x)
print("HERE")

q[0]=a0
print(q)

w1 = q[1] * 2
print(w1)
print(type(w1))

w2 = 3 * q[1]
print(w2)
print(type(w2))

w3 = q[1] + 2
print(w3)
print(type(w3))


for i in range(0,n):
   for j in range(i+1,n):
       cp.post(minicpp.notEqual(q[i],q[j],0),True)
       cp.post(minicpp.notEqual(q[i],q[j],i-j),True)
       cp.post(minicpp.notEqual(q[i],q[j],j-i),True)

print("Starting search...")
search = minicpp.DFSearch(cp,minicpp.firstFail(cp,q))
search.onSolution(lambda : print(q))
stat = search.solve()
print(stat)
