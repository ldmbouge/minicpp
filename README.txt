# minicpp

A C++ codebase for constraint programming that implements HADDOCK to allow MDD-based propagation.

# Compiling

To build the project, create a build folder with

mkdir build

Then navigate into the build folder and compile the code with

cd build
cmake ..
make

# Example models

There are example models included in the folder examples.  In examples/mdd, there are models that use MDD propagation.  The ones used in the paper are located at

examples/mdd/paper/amongNurse.cpp
examples/mdd/paper/allInterval.cpp

To run amongNurse from the build folder, run the following:

./amongNurse -w[0] -m[1] -r[2] -i[3] -c[4] -h[5]

Replacing [#] with numbered parameters.  The parameters represent the following:

-w[0] gives the maximum width of the MDD.  Should be a positive integer
-m[1] gives the model type.  Should be an integer from 0 to 5.
      0: Domain encoding of cumulative sums
      1: MDD encoding with among constraints (this is what was used in the paper)
      2: MDD encoding with sequence constraints
      3: MDD encoding with different sequence constraints (uses a different propagator)
      4: Domain encoding of cumulative sums with isMember
      5: MDD encoding with different among constraints (uses a different propagator than 1)
-r[2] gives the maximum reboot distance.  Should be a non-negative integer.
-i[3] gives the factor for calculating the maximum number of iterations with splitting to reach a fixpoint.  This is multiplied by the width.  Should be a positive integer.
-c[4] gives the constraint set used.  Should be an integer from 1 to 3.
      1: At most 6 out of 8 consecutive work days.  At least 22 out of 30 consecutive work days.
      2: At most 6 out of 9 consecutive work days.  At least 20 out of 30 consecutive work days.
      3: At most 7 out of 9 consecutive work days.  At least 22 out of 30 consecutive work days.
-h[5] gives the horizon.  Should be a positive integer of at least 30.

As an example, the command could look like this:

./amongNurse -w32 -m1 -r0 -i5 -c1 -h40

To run allInterval from the build folder, run the following:

./allInterval -n[0] -w[1] -m[2] -r[3] -i[4]

-n[0] gives the number of variables for the All-Interval series.  Should be a positive integer of at least 2.
-w[1] gives the maximum width of the MDD.  Should be a positive integer
-m[2] gives the model type.  Should be an integer from 0 to 3.
      0: Domain encoding with equalAbsDiff constraint
      1: Domain encoding with AbsDiff-Table constraint
      2: MDD encoding (this is what was used in the paper)
      3: Both the domain and MDD encodings from 1 and 2
-r[3] gives the maximum reboot distance.  Should be a non-negative integer.
-i[4] gives the factor for calculating the maximum number of iterations with splitting to reach a fixpoint.  This is multiplied by the width.  Should be a positive integer.

As an example, the command could look like this:

./allInterval -n11 -w32 -m2 -r2 -i5
