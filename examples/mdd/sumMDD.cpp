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

   int width = (argc >= 2 && strncmp(argv[1],"-w",2)==0) ? atoi(argv[1]+2) : 1;
   //   int mode  = (argc >= 3 && strncmp(argv[2],"-m",2)==0) ? atoi(argv[2]+2) : 1;

   CPSolver::Ptr cp  = Factory::makeSolver();
   auto vars = Factory::intVarArray(cp, 5, 0, 5);

   //auto mdd = new MDD(cp);
   auto mdd = new MDDRelax(cp,width);

   // Test 1: constants (lb, ub) as RHS  
   // vector<int> vals1 {1,2,3,4,5};
   // Factory::sumMDD(mdd->getSpec(), vars, vals1, 18, 19);

   // vector<int> vals2 {5, 4, 3, 2, 1};
   // Factory::sumMDD(mdd->getSpec(), vars, vals2, 18, 19);

   // vector<int> vals3 {7, 8, 11, 15, 4};
   // Factory::sumMDD(mdd->getSpec(), vars, vals3, 50, 65);

   // Test 2: variables as RHS
   auto z1 = Factory::makeIntVar(cp, 18, 19);  
   vector<int> vals1 {1,2,3,4,5};
   Factory::sumMDD(mdd->getSpec(), vars, vals1, z1);

   auto z2 = Factory::makeIntVar(cp, 18, 19);
   vector<int> vals2 {5, 4, 3, 2, 1};
   Factory::sumMDD(mdd->getSpec(), vars, vals2, z2);

   auto z3 = Factory::makeIntVar(cp, 50, 65);
   vector<int> vals3 {7, 8, 11, 15, 4};
   Factory::sumMDD(mdd->getSpec(), vars, vals3, z3);

   // Test 3: variable RHS with matrix (element) summation
   // auto z1 = Factory::makeIntVar(cp, 18, 19);  
   // vector<int> vals1 {1,2,3,4,5};
   // vector< vector<int> > valMatrix1;
   // for (unsigned int i=0; i<vars.size(); i++) {
   //   vector<int> tmpVals;
   //   for (int j=0; j<=5; j++) {
   //     tmpVals.push_back(j*vals1[i]);
   //   }
   //   valMatrix1.push_back(tmpVals);
   // }
   // Factory::sumMDD(mdd->getSpec(), vars, valMatrix1, z1);

   // auto z2 = Factory::makeIntVar(cp, 18, 19);  
   // vector<int> vals2 {5, 4, 3, 2, 1};
   // vector< vector<int> > valMatrix2;
   // for (unsigned int i=0; i<vars.size(); i++) {
   //   vector<int> tmpVals;
   //   for (int j=0; j<=5; j++) {
   //     tmpVals.push_back(j*vals2[i]);
   //   }
   //   valMatrix2.push_back(tmpVals);
   // }
   // Factory::sumMDD(mdd->getSpec(), vars, valMatrix2, z2);

   // auto z3 = Factory::makeIntVar(cp, 50, 65);
   // vector<int> vals3 {7, 8, 11, 15, 4};
   // vector< vector<int> > valMatrix3;
   // for (unsigned int i=0; i<vars.size(); i++) {
   //   vector<int> tmpVals;
   //   for (int j=0; j<=5; j++) {
   //     tmpVals.push_back(j*vals3[i]);
   //   }
   //   valMatrix3.push_back(tmpVals);
   // }
   // Factory::sumMDD(mdd->getSpec(), vars, valMatrix3, z3);
   
   cp->post(mdd);
   // mdd->saveGraph();

   
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
      
     search.onSolution([&vars,vals1,vals2,vals3]() {
	 std::cout << "Assignment:" << vars;

	 int value1 = 0;
	 for (unsigned int i=0; i<vars.size(); i++)
	   value1 += vals1[i]*vars[i]->min();

	 int value2 = 0;
	 for (unsigned int i=0; i<vars.size(); i++)
	   value2 += vals2[i]*vars[i]->min();

	 int value3 = 0;
	 for (unsigned int i=0; i<vars.size(); i++)
	   value3 += vals3[i]*vars[i]->min();

	 std::cout << " with values: " << value1 << ", " << value2 << ", " << value3 << std::endl;
       });
      
      
      auto stat = search.solve();
      cout << stat << endl;
      
    cp.dealloc();
    return 0;
}
