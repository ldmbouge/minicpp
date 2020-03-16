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
#include "mdd.hpp"
#include "mddrelax.hpp"
#include "RuntimeMonitor.hpp"

int main(int argc,char* argv[])
{
   using namespace std;
   using namespace Factory;
   CPSolver::Ptr cp  = Factory::makeSolver();
   auto vars = Factory::intVarArray(cp, 5, 0, 2);

   auto mdd = new MDD(cp);
   //auto mdd = new MDDRelax(cp,1);

   vector<int> vals {1,2,3,4,5};

   // Test 1: constant RHS
   //Factory::sumMDD(mdd->getSpec(), vars, vals, 15, 15);


   // Test 2: variable RHS
   auto z = Factory::makeIntVar(cp, 15, 15);
   //Factory::sumMDD(mdd->getSpec(), vars, vals, z);

   // Test 3: variable RHS with matrix (element) summation
   vector< vector<int> > valMatrix;
   vector<int> var0 {0,1,2};
   vector<int> var1 {0,2,4};
   vector<int> var2 {0,3,6};
   vector<int> var3 {0,4,8};
   vector<int> var4 {0,5,10};
   valMatrix.push_back(var0);
   valMatrix.push_back(var1);
   valMatrix.push_back(var2);
   valMatrix.push_back(var3);
   valMatrix.push_back(var4);
   Factory::sumMDD(mdd->getSpec(), vars, valMatrix, z);

   cp->post(mdd);
   
   DFSearch search(cp,[=]() {

      unsigned i = 0u;
      for(i=0u;i < vars.size();i++)
	if (vars[i]->size()> 1) break;
      auto x = i< vars.size() ? vars[i] : nullptr;
          
      if (x) {
	
	int c = x->min();
	
	return  [=] {
	  cp->post(x == c);}
	| [=] {
	  cp->post(x != c);};
      } else return Branches({});
       });
      
     search.onSolution([&vars,vals]() {
	 std::cout << "Assignment:" << vars;

	 int value = 0;
	 for (unsigned int i=0; i<vars.size(); i++)
	   value += vals[i]*vars[i]->min();

	 std::cout << " with value = " << value << std::endl;
       });
      
      
      auto stat = search.solve();
      cout << stat << endl;
      
    cp.dealloc();
    return 0;
}
