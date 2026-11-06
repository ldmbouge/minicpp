// eq20: port of Objective-CP's Clo/eq20/main.m (Copyright (c) 2015 NICTA,
// Laurent Michel and Pascal Van Hentenryck, MPL 2.0).
//
// Model: 7 variables in 0..10 and 20 linear equations
//    sum_j eqs[i][j] * x[j] == eqs[i][0]   (i = 0..19, j = 1..7).
// objcp annotates them with value consistency (vc); minicpp's sum is bounds
// consistent.
// Search: all solutions, first fail on x (objcp's default heuristic FF,
// followed by a first-fail labelArray that has nothing left to do).
//
// Usage:
//   eq20

#include <iostream>
#include <vector>
#include "solver.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"

int main(int argc, char* argv[])
{
   using namespace Factory;
   const int nbC = 20, nbV = 7;
   const int eqs[nbC][nbV + 1] = {
      {876370, -16105, 62397, -6704, 43340, 95100, -68610, 58301},
      {533909, 51637, 67761, 95951, 3834, -96722, 59190, 15280},
      {915683, 1671, -34121, 10763, 80609, 42532, 93520, -33488},
      {129768, 71202, -11119, 73017, -38875, -14413, -29234, 72370},
      {752447, 8874, -58412, 73947, 17147, 62335, 16005, 8632},
      {90614, 85268, 54180, -18810, -48219, 6013, 78169, -79785},
      {1198280, -45086, 51830, -4578, 96120, 21231, 97919, 65651},
      {18465, -64919, 80460, 90840, -59624, -75542, 25145, -47935},
      {1503588, -43277, 43525, 92298, 58630, 92590, -9372, -60227},
      {1244857, -16835, 47385, 97715, -12640, 69028, 76212, -81102},
      {1410723, -60301, 31227, 93951, 73889, 81526, -72702, 68026},
      {25334, 94016, -82071, 35961, 66597, -30705, -44404, -38304},
      {277271, -67456, 84750, -51553, 21239, 81675, -99395, -4254},
      {249912, -85698, 29958, 57308, 48789, -78219, 4657, 34539},
      {373854, 85176, -95332, -1268, 57898, 15883, 50547, 83287},
      {740061, -10343, 87758, -11782, 19346, 70072, -36991, 44529},
      {146074, 49149, 52871, -7132, 56728, -33576, -49530, -62089},
      {251591, -60113, 29475, 34421, -76870, 62646, 29278, -15212},
      {22167, 87059, -29101, -5513, -21219, 22128, 7276, 57308},
      {821228, -76706, 98205, 23445, 67921, 24111, -48614, -41906}};

   CPSolver::Ptr cp = makeSolver();
   auto x = intVarArray(cp, nbV, 0, 10);          // x[j-1] is objcp's x[j]
   for (int i = 0; i < nbC; i++) {
      std::vector<var<int>::Ptr> terms;
      for (int j = 1; j <= nbV; j++)
         terms.push_back(x[j - 1] * eqs[i][j]);
      cp->post(sum(terms, eqs[i][0]));
   }

   DFSearch search(cp, firstFail(cp, x));
   search.onSolution([&x] { std::cout << "Sol: " << x << std::endl; });
   auto stat = search.solve();
   std::cout << stat << std::endl;
   return 0;
}
