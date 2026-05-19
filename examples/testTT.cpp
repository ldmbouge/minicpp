/*
 * mini-cp is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License  v3
 * as published by the Free Software Foundation.
 *
 * mini-cp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY.
 * See the GNU Lesser General Public License  for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with mini-cp. If not, see http://www.gnu.org/licenses/lgpl-3.0.en.html
 *
 * Copyright (c)  2018. by Laurent Michel, Pierre Schaus, Pascal Van Hentenryck
 *
 * Example Pierre Schaus, Laurent Michel, P. Van Hentenryck
 */


#include <iostream>
#include <limits>
#include "solver.hpp"
#include "intvar.hpp"
#include "search.hpp"
#include "ttable.hpp"

int test1() {
   using namespace std;
   using namespace Factory;
   CPSolver::Ptr cp  = Factory::makeSolver();

   auto s = Factory::intVarArray(cp, 5, 5);
   vector<int> d(5, 1);
   vector<int> r(5, 100);
   cout << d << "\n" << r << "\n";   
   cp->post(new CumulativeTT(s, d, r, 100));

   DFSearch search(cp,firstFail(cp, s));
   search.onSolution([&s]() {
      std::cout << "sol = " << s << std::endl;
   });

   auto stat = search.solve();
   assert(stat.numberOfSolutions()==120);
   std::cout << stat << std::endl;
   cp.dealloc();
   return 0;
}

int test2() {
   using namespace std;
   using namespace Factory;
   CPSolver::Ptr cp  = Factory::makeSolver();

   auto s = Factory::intVarArray(cp, 2, 10);
   vector<int> d {5,5};
   vector<int> r {1,1};
   cout << d << "\n" << r << "\n";
   cp->post(new CumulativeTT(s, d, r, 1));
   cp->post(s[0] == 0);
   assert(s[1]->min() == 5);
   return 0;   
}

int test3() {
   using namespace std;
   using namespace Factory;
   CPSolver::Ptr cp  = Factory::makeSolver();

   auto s = Factory::intVarArray(cp, 2, 10);
   vector<int> d {5,5};
   vector<int> r {1,1};
   cout << d << "\n" << r << "\n";
   cp->post(new CumulativeTT(s, d, r, 1));
   cp->post(s[0] == 5);
   assert(s[1]->max() == 0);
   return 0;   
}

std::vector<int> discreteProfile(std::vector<Profile::Rectangle> &rects) {
  int min = std::numeric_limits<int>::max();
  int max = std::numeric_limits<int>::min();  
  for (const auto &r : rects) 
    if (r.getHeight() > 0) {
       min = std::min(min, r.getStart());
       max = std::max(max, r.getEnd());
    }
  std::vector<int> heights(max-min);
  for (const auto &r : rects)
    if (r.getHeight() > 0)
      for (int i = r.getStart(); i < r.getEnd(); i++)
         heights[i - min] += r.getHeight();
  return heights;
}


int testCapaOk() {
   using namespace std;
   using namespace Factory;
   CPSolver::Ptr cp  = Factory::makeSolver();

   auto s = Factory::intVarArray(cp, 2, 10);
   vector<int> d {5,10,3,6,1};
   vector<int> r {3,7,1,4,8};
   cout << d << "\n" << r << "\n";
   cp->post(new CumulativeTT(s, d, r, 12));

   DFSearch search(cp,firstFail(cp, s));
   search.onSolution([&s, &d, &r]() {
      vector<Profile::Rectangle> rects;
      for (auto i = 0u; i < s.size(); i++) {
         auto start = s[i]->min();
         auto end = start + d[i];
         auto h = r[i];
         rects.emplace_back(Profile::Rectangle(start,end,h));
      }
      vector<int> dp = discreteProfile(rects);
      for (int h : dp)
         assert(h <= 12);
   });

   search.solve();
   return 0;   
}  

int main() {
    test1();
    test2();
    test3();
    testCapaOk();    
}
