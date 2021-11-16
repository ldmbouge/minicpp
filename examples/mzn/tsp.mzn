% Travelling Salesman Problem
include "globals.mzn";

int: n;
array[1..n,1..n] of int: distanceMatrix;
array[1..n] of var 1..n: succ;
array[1..n] of var 0..1000: distSucc;
var 0..n*1000: obj;

constraint circuit(succ);
constraint forall (i in 1..n)(distanceMatrix[i,succ[i]] = distSucc[i]);
constraint obj = sum(distSucc);

solve :: int_search(succ, first_fail, indomain_min, complete) minimize obj;

output ["solution = \(succ)\nobjective = \(obj)\n"]; 