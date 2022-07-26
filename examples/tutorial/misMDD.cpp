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
 * Copyright (c)  2018. by Laurent Michel, Willem Jan van Hoeve, Pierre Schaus, Pascal Van Hentenryck
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
#include "range.hpp"

int main(int argc,char* argv[])
{
   using namespace std;
   using namespace Factory;
   CPSolver::Ptr cp  = Factory::makeSolver();
   auto x = Factory::intVarArray(cp, 5, 0, 1);
   auto z = Factory::makeIntVar(cp,0,10000);
   auto mdd = Factory::makeMDDRelax(cp,2);
   std::vector<int> coef = {5,4,2,6,8};
   mdd->post(sum(x,coef,z));
   mdd->post(sum({x[0],x[1]},0, 1));
   mdd->post(sum({x[0],x[4]},0, 1));
   mdd->post(sum({x[1],x[2]},0, 1));
   mdd->post(sum({x[1],x[3]},0, 1));
   mdd->post(sum({x[2],x[3]},0, 1));
   mdd->post(sum({x[3],x[4]},0, 1));
   cp->post(mdd);
   auto obj = Factory::maximize(z);
   std::cout << "VARS: " << x << "\t Z=" << z <<  "\n";
   
   DFSearch search(cp,[=]() {
      return indomain_max(cp,selectFirst(x,[](const auto& xi) { return xi->size() > 1;}));
   });

   // DFSearch search(cp,[=]() {
   //    //auto xk = selectFirst(x,[](const auto& xi) { return xi->size() > 1;});
   //    auto k = selectMax(Range<int>(0,4),
   //                       [&](const auto& i) { return x[i]->size() > 1;},
   //                       [&](const auto& i) { return coef[i];},-1); // largest coefficient first
   //    auto xk = k!=-1 ? x[k] : nullptr;
   //    //return indomain_max(cp,xk);
   //    if (xk) {
   //       int c = xk->max();
   //       return  [=] {
   //          std::cout << "choice  <" << xk << " == " << c << ">" << std::endl;
   //          cp->post(xk == c);
   //       }
   //          | [=] {
   //             std::cout << "choice  <" << xk << " != " << c << ">" << std::endl;
   //             cp->post(xk != c);
   //          };
   //    } else return Branches({});
   // });   
   search.onSolution([&x,&z]() {
      std::cout << "Assignment:" << x << "\t OBJ:" << z << "\n";
   });        
   auto stat = search.optimize(obj);
   cout << stat << endl;
   cp.dealloc();
   return 0;
}
