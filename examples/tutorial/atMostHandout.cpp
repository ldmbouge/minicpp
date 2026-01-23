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

namespace  Factory {
   /**
    * atMost MDD LTS specification.
    * @param m the propagator to hold the MDD
    * @param vars the array of variables subjected to the upper bound constraints
    * @param ub the upper bounds for every value susceptible to appear in `vars`
    *
    * The requirement is: FORALL k in range(ub) : |{x in vars | sigma(x) = k}| <= ub_k.
    * The MDD uses *only* downward information to filter.
    */
   MDDCstrDesc::Ptr atMostMDD(MDD::Ptr m,const Factory::Veci& vars,const std::map<int,int>& ub)
   {
      MDDSpec& spec = m->getSpec();
      //const auto [minFDom,minLDom] = domRange(vars); // the full range of values that can appear in vars (union of domains)
      auto desc = spec.makeConstraintDescriptor(vars,"atMostMDD"); // the descriptor for the new LTS

      std::map<int,MDDPInt::Ptr> pd; // HINT: a map that associates values to properties
      // Your code goes here! The step below is the minimal spec.
      // Step 1: specify the state properties (& relaxations)
      // Step 2: specify the down transitions
      // Step 3: specify the arc existence 
                                           
      return desc;
   }

   /**
    * atMost MDD LTS specification (stronger filtering).
    * @param m the propagator to hold the MDD
    * @param vars the array of variables subjected to the upper bound constraints
    * @param ub the upper bounds for every value susceptible to appear in `vars`
    *
    * The requirement is: FORALL k in range(ub) : |{x in vars | sigma(x) = k}| <= ub_k.
    * The MDD uses *both* downward and upward information to filter.
    */
   MDDCstrDesc::Ptr atMostMDD2(MDD::Ptr m,const Factory::Veci& vars,const std::map<int,int>& ub)
   {
      MDDSpec& spec = m->getSpec();
      //const auto [minFDom,minLDom] = domRange(vars);
      auto desc = spec.makeConstraintDescriptor(vars,"atMostMDD");

      std::map<int,MDDPInt::Ptr> pd; // HINT: a map that associates values to properties
      // Your code goes here! The step below is the minimal spec.
      // Step 1: specify the state properties (& relaxations) for both UP and DOWN
      // Step 2: specify the down/up transitions 
      // Step 3: specify the arc existence 
                                           

      return desc;
   }
}

int main(int argc,char* argv[])
{
   using namespace std;
   using namespace Factory;
   CPSolver::Ptr cp  = Factory::makeSolver();
   int min = 1; int max = 10;
   auto v = Factory::intVarArray(cp,20, min, max); // [v0,v1,....,v18,v19] (array of 20 variables with D(vi) = [1..10])
   auto x0  = Factory::intVarArray(cp,15,[&](int i) { return v[i];});   // the first 15 variables
   auto x1  = Factory::intVarArray(cp,15,[&](int i) { return v[i+5];}); // the last 15 variables
   auto mdd = Factory::makeMDDRelax(cp,8); // a relaxed MDD propagator (width = 8) to hold 2 atMost constraints
   std::map<int,int> boundsC1 = {  // bounds for first atMost
      {1,3},{2,3},{3,3},
      {4,3},{5,3},{6,3},
      {7,3},{8,3},{9,10},
      {10,0}
   };
   std::map<int,int> boundsC2 = { // bounds for second atMost
      {1,0},{2,1},{3,1},
      {4,0},{5,0},{6,2},
      {7,1},{8,2},{9,3},
      {10,10}
   };
   TRYFAIL {
      mdd->post(atMostMDD(mdd,x0,boundsC1));   
      mdd->post(atMostMDD(mdd,x1,boundsC2));
      cp->post(mdd);
   } ONFAIL {
      std::cout << "Infeasible model...\n";
      return 1;
   }
   ENDFAIL;
      
   DFSearch search(cp,[=]() {
      return indomain_min(cp,selectFirst(v,[](const auto& x) { return x->size() > 1;}));
   });
      
   search.onSolution([&v]() {
      std::cout << "Assignment:" << v << "\n";
   });      
      
   auto stat = search.solve([](const SearchStatistics& stats) {
      return stats.numberOfSolutions() > 0;
   });

   cout << stat << endl;
   cp.dealloc();
   return 0;
}
