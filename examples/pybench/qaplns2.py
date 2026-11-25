#!/usr/bin/env python3
# Port of examples/basic/qaplns2.cpp: quadratic assignment by LNS, with the lns
# combinator:
#  - capture records each solution;
#  - freeze fixes a random fragment (about `percent`%) of the last solution;
#    lns skips it until a first solution exists, and a fragment that fails at
#    the root simply starts the next iteration;
#  - each iteration runs firstFail (in C++) under a failure limit;
#  - the search-wide limit stops after nRestarts neighbourhoods.
#
# With an explicit seed, the random draws are the same as the C++ version's
# (std::mt19937 + std::uniform_int_distribution(0, 99) as in libc++), so both
# runs explore the same neighbourhoods and print the same output.
#
# Usage: qaplns2.py [nRestarts (1000)] [failureLimit (100)] [percent (25)] [seed (random)]
import os, random, sys
from minicpp import *

nRestarts    = int(sys.argv[1]) if len(sys.argv) > 1 else 1000
failureLimit = int(sys.argv[2]) if len(sys.argv) > 2 else 100
percent      = int(sys.argv[3]) if len(sys.argv) > 3 else 25
seed         = int(sys.argv[4]) if len(sys.argv) > 4 else random.SystemRandom().getrandbits(32)

class CppDraw:
    """Draws 0..99 exactly as std::uniform_int_distribution<>(0, 99) over
    std::mt19937(seed) does with libc++."""
    def __init__(self, seed):
        # std::mt19937(seed) seeding (init_genrand); Python's Random is the same
        # MT19937, only its own seed() initializes the state differently.
        mt = [seed & 0xffffffff]
        for i in range(1, 624):
            mt.append((1812433253 * (mt[-1] ^ (mt[-1] >> 30)) + i) & 0xffffffff)
        self.rng = random.Random()
        self.rng.setstate((3, tuple(mt) + (624,), None))
    def draw(self):
        # libc++: take the low 7 bits (2^7 >= 100) of a 32-bit output and
        # reject values >= 100.
        while True:
            u = self.rng.getrandbits(32) & 0x7f
            if u < 100:
                return u

here = os.path.dirname(os.path.abspath(__file__))
with open(os.path.join(here, "../../data/qap/qap.txt")) as f:
    data = iter(int(t) for t in f.read().split())
n = next(data)
w = [[next(data) for j in range(n)] for i in range(n)]
d = [[next(data) for j in range(n)] for i in range(n)]

cp = makeSolver()
x = intVarArray(cp, n, n)
cp.post(allDifferent(x))
wDist = [w[i][j] * element(d, x[i], x[j]) for i in range(n) for j in range(n)]
obj = minimize(sum(wDist))

# capture: the solution DFS is about to report (every x is bound at a leaf).
best = [0] * n
def capture():
    for i in range(n):
        best[i] = x[i].min

# freeze: fix each x[j] to its value in the last solution with probability percent%.
gen = CppDraw(seed)
nbRestarts = 0
def freeze():
    global nbRestarts
    if nbRestarts % 10 == 0:
        print("restart number #%d" % nbRestarts)
    nbRestarts += 1
    for j in range(n):
        if gen.draw() < percent:
            cp.post(x[j] == best[j])   # may fail: lns then starts the next iteration

search = DFSearch(cp, lns(cp, capture, freeze, firstFail(cp, x),
                          lambda s: s.numberOfFailures() >= failureLimit))

# The best value is recorded here: once optimize returns, the search is back at
# the root and obj.value() no longer reflects the incumbent.
bestValue = None
def onSolution():
    global bestValue
    bestValue = obj.value()
    print("objective = %d\t(restart %d)" % (bestValue, nbRestarts))
search.onSolution(onSolution)

# lns runs until stopped: stop once nRestarts neighbourhoods have been started.
stat = search.optimize(obj, lambda s: nbRestarts >= nRestarts)

print("Restarts: %d" % nbRestarts)
if bestValue is not None:
    # best is the last captured solution, i.e. the incumbent (each one improves).
    print("Best: %d" % bestValue)
    print("Assignment: " + " ".join(str(v) for v in best))
else:
    print("Best: none found")
print(stat)
