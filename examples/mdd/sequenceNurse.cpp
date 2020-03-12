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

  /***
   * Nurse scheduling problem from [Bergman, Cire, van Hoeve, JAIR 2014].
   * Determine the work schedule for a single nurse over time horizon {1..H}.
   *
   * Variable x[i] represents the assignment for day i, with domain {D, E, N, O}
   * (Day, Evening, Night, Off).
   *
   * Constraints:
   *  - at least 20 work shifts every 28 days:             Sequence(X, 28, 20, 28, {D, E, N})
   *  - at least 4 off-days every 14 days:                 Sequence(X, 14, 4, 14, {O})
   *  - between 1 and 4 night shifts every 14 days:        Sequence(X, 14, 1, 4, {N})
   *  - between 4 and 8 evening shifts every 14 days:      Sequence(X, 14, 4, 8, {E})
   *  - night shifts cannot appear on consecutive days:    Sequence(X, 2, 0, 1, {N})
   *  - between 2 and 4 evening/night shifts every 7 days: Sequence(X, 7, 2, 4, {E, N})
   *  - at most 6 work shifts every 7 days:                Sequence(X, 7, 0, 6, {D, E, N})
   *
   * The planning horizon H ranges from 40 to 100 days.
   ***/
  
  int H = 40; // time horizon (number of days)

  // vars[i] is shift on day i
  // mapping: 0 = Day, 1 = Evening, 2 = Night, 3 = Off
  auto vars = Factory::intVarArray(cp, H, 0, 3); 

  auto mdd = new MDDRelax(cp,relaxSize);

  //  - at least 20 work shifts every 28 days:             Sequence(X, 28, 20, 28, {D, E, N})
  std::set<int> S = {0,1,2};
  Factory::seqMDD(mdd->getSpec(), vars, 28, 20, 28, S);

  //  - at least 4 off-days every 14 days:                 Sequence(X, 14, 4, 14, {O})
  S = {3};
  Factory::seqMDD(mdd->getSpec(), vars, 14, 4, 14, S);

  //  - between 1 and 4 night shifts every 14 days:        Sequence(X, 14, 1, 4, {N})
  S = {2};
  Factory::seqMDD(mdd->getSpec(), vars, 14, 1, 4, S);

  //  - between 4 and 8 evening shifts every 14 days:      Sequence(X, 14, 4, 8, {E})
  S = {1};
  Factory::seqMDD(mdd->getSpec(), vars, 14, 4, 8, S);

  //  - night shifts cannot appear on consecutive days:    Sequence(X, 2, 0, 1, {N})
  S = {2};
  Factory::seqMDD(mdd->getSpec(), vars, 2, 0, 1, S);

  //  - between 2 and 4 evening/night shifts every 7 days: Sequence(X, 7, 2, 4, {E, N})
  S = {1,2};
  Factory::seqMDD(mdd->getSpec(), vars, 7, 2, 4, S);

  //  - at most 6 work shifts every 7 days:                Sequence(X, 7, 0, 6, {D, E, N})
  S = {0,1,2};
  Factory::seqMDD(mdd->getSpec(), vars, 7, 0, 6, S);

  cp->post(mdd);
  
  DFSearch search(cp,[=]() {
      unsigned i = 0u;
      for(i=0u;i < vars.size();i++)
	if (vars[i]->size()> 1) break;
      auto x = i< vars.size() ? vars[i] : nullptr;
      /*
      auto x = selectMin(vars,
			 [](const auto& x) { return x->size() > 1;},
			 [](const auto& x) { return x->size();});
      */
      if (x) {
	int c = x->min();
	
	return  [=] {
	  cp->post(x == c);}
	| [=] {
	  cp->post(x != c);};
      } else return Branches({});
    });
  
  int cnt = 0;
  search.onSolution([&vars,&cnt]() {
      //cnt++;
      //std::cout << " " << cnt;
       std::cout << "Assignment:" << std::endl;
       std::cout << vars << std::endl;
    });
  
  auto stat = search.solve([](const SearchStatistics& stats) {
      //return stats.numberOfSolutions() > INT_MAX;
                              return stats.numberOfSolutions() > 0;
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
