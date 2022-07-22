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
 */

#include <iostream>
#include <iomanip>
#include "solver.hpp"
#include "trailable.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"
#include "mddrelax.hpp"
#include "mddConstraints.hpp"
#include "RuntimeMonitor.hpp"

int main(int argc,char* argv[])
{
   using namespace std;
   using namespace Factory;
   CPSolver::Ptr cp  = Factory::makeSolver();
   auto x = Factory::intVarArray(cp, 5, 0, 1);
   auto z = Factory::makeIntVar(cp,0,10000);

   cp->post(sum(x,{5,4,2,6,8}) == z);
   cp->post(sum({x[0],x[1]}) <= 1);
   cp->post(sum({x[0],x[4]}) <= 1);
   cp->post(sum({x[1],x[2]}) <= 1);
   cp->post(sum({x[1],x[3]}) <= 1);
   cp->post(sum({x[2],x[3]}) <= 1);
   cp->post(sum({x[3],x[4]}) <= 1);

   auto obj = Factory::maximize(z);
   std::cout << "VARS: " << x << "\tZ=" << z << std::endl;
   
   DFSearch search(cp,[=]() {
      return indomain_max(cp,selectFirst(x,
                                         [](const auto& xi) { return xi->size() > 1;}));
   });

   search.onSolution([&x,&z]() {
      std::cout << "Assignment:" << x << "\t OBJ:" << z << std::endl;
   });        
   auto stat = search.optimize(obj);
   cout << stat << endl;
   cp.dealloc();
   return 0;
}
