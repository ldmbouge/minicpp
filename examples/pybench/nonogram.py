#!/usr/bin/env python3
# Port of examples/basic/nonogram.cpp (objcp's nonogram, puzzle P200, 25x25):
# each row and column is a regular constraint over an automaton built from its
# clue. Cells are labelled in objcp's order, trying "filled" (1) first.
import builtins
from minicpp import *

builtins_sum = builtins.sum      # minicpp's sum shadows the built-in one

def clueAutomaton(cp, pattern):
    """objcp's make_transition_matrix: the automaton accepting the 0/1 strings
    whose blocks of 1s have the lengths in pattern, in order. The states follow
    the template w b^p1 w b^p2 w ... w: a white state loops on w and moves to the
    next state on b; a black state moves on the next symbol of the template. The
    last two states are final. An empty clue gets a single white state."""
    w, b = 0, 1
    if not pattern:
        return automaton(cp, 0, 1, 0, 0, 0, {0}, [(0, w, 0)])
    nbStates = len(pattern) + builtins_sum(pattern) + 1
    tmp = [w]
    for p in pattern:
        tmp += [b] * p + [w]
    assert len(tmp) == nbStates
    t = []
    for k in range(nbStates):
        if tmp[k] == w:
            t.append((k, w, k))
            if k < nbStates - 1:
                t.append((k, b, k + 1))
        else:
            t.append((k, b if tmp[k + 1] == b else w, k + 1))
    return automaton(cp, 0, 1, 0, nbStates - 1, 0, {nbStates - 1, nbStates - 2}, t)

def checkRule(cp, rule, y):
    """objcp's checkRule: drop the 0 padding of a clue and post regular."""
    cp.post(regular(y, clueAutomaton(cp, [r for r in rule if r > 0])))

rowRules = [
    [0, 0, 0, 0, 2, 2, 3],
    [0, 0, 4, 1, 1, 1, 4],
    [0, 0, 4, 1, 2, 1, 1],
    [4, 1, 1, 1, 1, 1, 1],
    [0, 2, 1, 1, 2, 3, 5],
    [0, 1, 1, 1, 1, 2, 1],
    [0, 0, 3, 1, 5, 1, 2],
    [0, 3, 2, 2, 1, 2, 2],
    [2, 1, 4, 1, 1, 1, 1],
    [0, 2, 2, 1, 2, 1, 2],
    [0, 1, 1, 1, 3, 2, 3],
    [0, 0, 1, 1, 2, 7, 3],
    [0, 0, 1, 2, 2, 1, 5],
    [0, 0, 3, 2, 2, 1, 2],
    [0, 0, 0, 3, 2, 1, 2],
    [0, 0, 0, 0, 5, 1, 2],
    [0, 0, 0, 2, 2, 1, 2],
    [0, 0, 0, 4, 2, 1, 2],
    [0, 0, 0, 6, 2, 3, 2],
    [0, 0, 0, 7, 4, 3, 2],
    [0, 0, 0, 0, 7, 4, 4],
    [0, 0, 0, 0, 7, 1, 4],
    [0, 0, 0, 0, 6, 1, 4],
    [0, 0, 0, 0, 4, 2, 2],
    [0, 0, 0, 0, 0, 2, 1],
]
colRules = [
    [0, 0, 1, 1, 2, 2],
    [0, 0, 0, 5, 5, 7],
    [0, 0, 5, 2, 2, 9],
    [0, 0, 3, 2, 3, 9],
    [0, 1, 1, 3, 2, 7],
    [0, 0, 0, 3, 1, 5],
    [0, 7, 1, 1, 1, 3],
    [1, 2, 1, 1, 2, 1],
    [0, 0, 0, 4, 2, 4],
    [0, 0, 1, 2, 2, 2],
    [0, 0, 0, 4, 6, 2],
    [0, 0, 1, 2, 2, 1],
    [0, 0, 3, 3, 2, 1],
    [0, 0, 0, 4, 1, 15],
    [1, 1, 1, 3, 1, 1],
    [2, 1, 1, 2, 2, 3],
    [0, 0, 1, 4, 4, 1],
    [0, 0, 1, 4, 3, 2],
    [0, 0, 1, 1, 2, 2],
    [0, 7, 2, 3, 1, 1],
    [0, 2, 1, 1, 1, 5],
    [0, 0, 0, 1, 2, 5],
    [0, 0, 1, 1, 1, 3],
    [0, 0, 0, 4, 2, 1],
    [0, 0, 0, 0, 0, 3],
]
rows, rowRuleLen = len(rowRules), len(rowRules[0])
cols, colRuleLen = len(colRules), len(colRules[0])

cp = makeSolver()
x = [intVarArray(cp, cols, 0, 1) for r in range(rows)]
for i in range(rows):
    checkRule(cp, rowRules[i], [x[i][k] for k in range(cols)])
for i in range(cols):
    checkRule(cp, colRules[i], [x[k][i] for k in range(rows)])

# Cells in objcp's labelling order.
if rows * rowRuleLen < cols * colRuleLen:
    order = [x[r][c] for r in range(rows) for c in range(cols)]
else:
    order = [x[r][c] for c in range(cols) for r in range(rows)]

def branch():
    for v in order:
        if not v.isBound:
            return Branches(lambda: cp.post(v == 1), lambda: cp.post(v == 0))
    return Branches()

def show():
    for row in x:
        print("|" + "".join(" " if v.min == 0 else "#" for v in row) + "|")

search = DFSearch(cp, branch)
search.onSolution(show)
stat = search.solve()
print(stat)
