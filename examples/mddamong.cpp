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
#include <set>
#include "solver.hpp"
#include "trailable.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"
#include "mdd.hpp"
#include "mddConstraints.hpp"


int main(int argc,char* argv[])
{
    
  using namespace std;
  using namespace Factory;
  CPSolver::Ptr cp  = Factory::makeSolver();
    
  auto iv = Factory::intVarArray(cp, 6, 1, 5);
     
  std::set<int> values_5 = {5};
  std::set<int> values_2 = {2};
  std::set<int> values_rem = {1,3,4};
     
  MDD* mdd = new MDD(cp);
  Factory::amongMDD(mdd->getSpec(), iv, 1, 2, values_5);
  Factory::amongMDD(mdd->getSpec(), iv, 1, 2, values_2);
  cp->post(mdd);
  mdd->saveGraph();
    
  DFSearch search(cp,[=]() {
      auto x = selectMin(iv,
			 [](const auto& x) { return x->size() > 1;},
			 [](const auto& x) { return x->size();});
        
      if (x) {            
	int c = x->min();
            
	return  [=] {cp->post(x == c);}
	| [=] {cp->post(x != c);};
      } else return Branches({});
    });
    
  auto stat = search.solve();
  cout << stat << endl;
    
  cp.dealloc();
  return 0;
}

