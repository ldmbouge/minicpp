% Use this editor as a MiniZinc scratch book
include "globals.mzn";

set of int : R0 = 1..200;
set of int : R1 = 1..9;
set of int : cv1 = {2};
set of int : cv2 = {3};
set of int : cv3 = {4};
set of int : cv4 = {5};

array[R1] of int : cover = [1,2,3,4,5,6,7,8,9];
array[R1] of int : low   = [0,2,2,3,3,0,0,0,0];
array[R1] of int : up    = [200,5,5,5,5,200,200,200,200];

array[R0] of var R1 : vars;

constraint at_most(5,vars,2) /\ at_least(2,vars,2);
constraint at_most(5,vars,3) /\ at_least(2,vars,3);  
constraint at_most(5,vars,4) /\ at_least(3,vars,4);  
constraint at_most(5,vars,5) /\ at_least(3,vars,5);  

%constraint global_cardinality_low_up(vars,cover,low,up);
  
solve :: int_search(vars, input_order, indomain_min)
  satisfy;

output ["vars = \(vars)\n"];