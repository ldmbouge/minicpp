#!/usr/bin/env python3
# Port of examples/basic/smory.cpp: SEND + MORE = MONEY.
from minicpp import *

cp = makeSolver()
S, E, N, D, M, O, R, Y = [makeIntVar(cp, 0, 9) for _ in range(8)]
x = [S, E, N, D, M, O, R, Y]
names = "SENDMORY"
cp.post(allDifferent(x))
cp.post(M != 0)
cp.post(S != 0)

send, more, money = [S, E, N, D], [M, O, R, E], [M, O, N, E, Y]
c1, c2 = [1000, 100, 10, 1], [10000, 1000, 100, 10, 1]
terms = [send[i] * c1[i] for i in range(4)] \
      + [more[i] * c1[i] for i in range(4)] \
      + [money[i] * -c2[i] for i in range(5)]
cp.post(sum(terms, 0))

search = DFSearch(cp, firstFail(cp, x))
search.onSolution(lambda: print("Sol:", " ".join(f"{names[i]}={x[i].min}" for i in range(len(x)))))
stat = search.solve()
print(stat)
