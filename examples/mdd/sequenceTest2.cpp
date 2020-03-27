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
#include "mddConstraints.hpp"
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


void addCumulSeq(CPSolver::Ptr cp, const Veci& vars, int N, int L, int U, const std::set<int> S) {

  int H = vars.size();
  
  auto cumul = Factory::intVarArray(cp, H+1, 0, H); 
  cp->post(cumul[0] == 0);
    
  auto boolVar = Factory::boolVarArray(cp, H);
  for (int i=0; i<H; i++) {
    cp->post(isMember(boolVar[i], vars[i], S));
  }
    
  for (int i=0; i<H; i++) {
    cp->post(equal(cumul[i+1], cumul[i], boolVar[i]));
  }
    
  for (int i=0; i<H-N+1; i++) {
    cp->post(cumul[i+N] <= cumul[i] + U);
    cp->post(cumul[i+N] >= cumul[i] + L);
  }
  
}


void buildModel(CPSolver::Ptr cp, int relaxSize, int mode)
{

  auto vars = Factory::intVarArray(cp, 4, 0, 2);

  /***
   *  In this example, both domain propagation on cumulative sums and the MDD propagation
   *  will deduce that vars[3] == 0.  This will trigger more domain propagation (the bounds
   *  on the cumulative variables get updated, which results in vars[2] == 1.
   *  The seqMDD3, however, is almost getting there, but somehow the changes in the state
   *  variables are not triggering a propagation event.  This means that the fixpoint 
   *  terminates earlier.
   ***/
  
  int Q1 = 3;
  int L1 = 1;
  int U1 = 2;
  std::set<int> S1 = {1};

  int Q2 = 4;
  int L2 = 2;
  int U2 = 3;
  std::set<int> S2 = {1,2};

  cp->post( vars[0] == 1 );
  cp->post( vars[1] == 2 );
  cp->post( vars[2] != 0 );
  
  if (mode == 0) {
    cout << "Cumulative Sums encoding" << endl; 
    addCumulSeq(cp, vars, Q1, L1, U1, S1);
    addCumulSeq(cp, vars, Q2, L2, U2, S2);
  }
  else if (mode == 1) {
    cout << "SeqMDD1 encoding" << endl; 
 
    auto mdd = new MDDRelax(cp,relaxSize);
    Factory::seqMDD(mdd->getSpec(), vars, Q1, L1, U1, S1);
    Factory::seqMDD(mdd->getSpec(), vars, Q2, L2, U2, S2);

    cp->post(mdd);
    mdd->saveGraph();
  }
  else if (mode == 2) {
    cout << "SeqMDD2 encoding" << endl; 
 
    auto mdd = new MDDRelax(cp,relaxSize);
    Factory::seqMDD2(mdd->getSpec(), vars, Q1, L1, U1, S1);
    Factory::seqMDD2(mdd->getSpec(), vars, Q2, L2, U2, S2);

    cp->post(mdd);
    mdd->saveGraph();
  }
  else if (mode == 3) {
    cout << "SeqMDD3 encoding" << endl; 
 
    auto mdd = new MDDRelax(cp,relaxSize);
    Factory::seqMDD3(mdd->getSpec(), vars, Q1, L1, U1, S1);
    Factory::seqMDD3(mdd->getSpec(), vars, Q2, L2, U2, S2);

    cp->post(mdd);
    mdd->saveGraph();
  }
  
  DFSearch search(cp,[=]() {

      unsigned i;
      for(i=0u;i< vars.size();i++)
	if (vars[i]->size() > 1)
	  break;

      auto x = i < vars.size() ? vars[i] : nullptr;
      if (x) {
	std::cout << "vars[" << i << "]";
      }
      if (x) {
	int c = x->min();
	
	return  [=] {
	  std::cout << " == " << c << std::endl;
	  cp->post(x == c);
	}
	| [=] {
	  std::cout << " != " << c << std::endl;
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

   // mode: 0 (cumulative sums), 1 (MDD)
   
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
