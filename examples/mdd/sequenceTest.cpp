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

#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <regex>
#include <fstream>      // std::ifstream
#include <iomanip>
#include <iostream>
#include <set>
#include <tuple>
#include <limits>
#include <iterator>
#include <climits>

#include "solver.hpp"
#include "trailable.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"
#include "mdd.hpp"
#include "RuntimeMonitor.hpp"
#include "mddrelax.hpp"
#include "matrix.hpp"

using namespace Factory;
using namespace std;

Veci all(CPSolver::Ptr cp,const set<int>& over, std::function<var<int>::Ptr(int)> clo)
{
   auto res = Factory::intVarArray(cp, (int) over.size());
   int i = 0;
   for(auto e : over){
      res[i++] = clo(e);
   }
   return res;
}


void buildModel(CPSolver::Ptr cp, int relaxSize, int mode)
{

  auto vars = Factory::intVarArray(cp, 10, 0, 1); 

  auto mdd = new MDDRelax(cp,relaxSize);

  std::set<int> S = {1};

  // This sequence constraint gives no error: 
  //Factory::seqMDD(mdd->getSpec(), vars, 5, 2, 3, S);

  // But this one gives an infeasible during post (irrespective of S).
  // There is something wrong with the lower bound: 
  //Factory::seqMDD(mdd->getSpec(), vars, 5, 3, 3, S);
  
  Factory::seqMDD2(mdd->getSpec(), vars, 5, 2, 3, S);

  cp->post(mdd);
  mdd->saveGraph();
  
  DFSearch search(cp,[=]() {

                        unsigned i;
                        for(i=0u;i< vars.size();i++)
                           if (vars[i]->size() > 1)
                              break;
                        auto x = i < vars.size() ? vars[i] : nullptr;
                        /*                        
      auto x = selectMin(vars,
			 [](const auto& x) { return x->size() > 1;},
			 [](const auto& x) { return x->size();});
                        */
      if (x) {
	int c = x->min();
	
	return  [=] {
                   cp->post(x == c);
                }
           | [=] {
                cp->post(x != c);
             };
      } else return Branches({});
    });
  
  search.onSolution([&vars]() {
      std::cout << "Assignment:" << vars << std::endl;
    });
  
  auto stat = search.solve([](const SearchStatistics& stats) {
      return stats.numberOfSolutions() > INT_MAX;
      // return stats.numberOfSolutions() > 0;
    }); 
  cout << stat << endl;
  
}

int main(int argc,char* argv[])
{
   int width = (argc >= 2 && strncmp(argv[1],"-w",2)==0) ? atoi(argv[1]+2) : 1;
   int mode  = (argc >= 3 && strncmp(argv[2],"-m",2)==0) ? atoi(argv[2]+2) : 1;

   // mode: 0 (standard summations), 1 (MDD)
   
   std::cout << "width = " << width << std::endl;
   std::cout << "mode = " << mode << std::endl;
   try {
      CPSolver::Ptr cp  = Factory::makeSolver();
      buildModel(cp, width, mode);
   } catch(Status s) {
      std::cout << "model infeasible during post" << std::endl;
   } catch (std::exception& e) {
      std::cerr << "Unable to find the file" << std::endl;
   }

   return 0;   
}
