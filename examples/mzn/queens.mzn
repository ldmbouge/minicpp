% N-Queens problem
include "globals.mzn";

int: n = 14;
array[1..n] of var 1..n: q;
array[1..n] of var 1..n+n: q_plus_i;
array[1..n] of var -n..n: q_minus_i;

constraint forall (i in 1..n)(q_plus_i[i] = q[i] + i);
constraint forall (i in 1..n)(q_minus_i[i] = q[i] - i);

constraint all_different(q);
constraint all_different(q_plus_i);
constraint all_different(q_minus_i);

solve :: int_search(  q, first_fail, indomain_min, complete) satisfy;

output ["queens = \(q)\n"];  
