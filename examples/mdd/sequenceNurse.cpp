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

std::string tab(int d) {
   std::string s = "";
   while (d--!=0)
      s = s + "  ";
   return s;
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

  /***
   * Nurse scheduling problem from [Bergman, Cire, van Hoeve, JAIR 2014].
   * Determine the work schedule for a single nurse over time horizon {1..H}.
   *
   * Variable x[i] represents the assignment for day i, with domain {D, E, N, O}
   * (Day, Evening, Night, Off).
   *
   * Constraints:
   *  - at least 4 off-days every 14 days:                 Sequence(X, 14, 4, 14, {O})
   *  - at least 20 work shifts every 28 days:             Sequence(X, 28, 20, 28, {D, E, N})
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
  // mapping: 0 = Off, 1 = Day, 2 = Evening, 3 = Night
  auto vars = Factory::intVarArray(cp, H, 0, 3);

  auto mdd = new MDDRelax(cp,relaxSize);

  if (mode == 0 ) {

    cout << "Cumulative Sums encoding" << endl;
    addCumulSeq(cp, vars, 14, 4, 14, {0});
    addCumulSeq(cp, vars, 28, 20, 28, {1,2,3});
    addCumulSeq(cp, vars, 14, 1, 4, {3});
    addCumulSeq(cp, vars, 14, 4, 8, {2});
    addCumulSeq(cp, vars, 2, 0, 1, {3});
    addCumulSeq(cp, vars, 7, 2, 4, {2,3});
    addCumulSeq(cp, vars, 7, 0, 6, {1,2,3});
      
  }
  else if (mode == 1) {

    cout << "seqMDD encoding" << endl;
      
    //  - at least 4 off-days every 14 days:                 Sequence(X, 14, 4, 14, {O})
    Factory::seqMDD(mdd->getSpec(), vars, 14, 4, 14, {0});

    //  - at least 20 work shifts every 28 days:             Sequence(X, 28, 20, 28, {D, E, N})
    Factory::seqMDD(mdd->getSpec(), vars, 28, 20, 28, {1,2,3});
  
    //  - between 1 and 4 night shifts every 14 days:        Sequence(X, 14, 1, 4, {N})
    Factory::seqMDD(mdd->getSpec(), vars, 14, 1, 4, {3});
    
    //  - between 4 and 8 evening shifts every 14 days:      Sequence(X, 14, 4, 8, {E})
    Factory::seqMDD(mdd->getSpec(), vars, 14, 4, 8, {2});
    
    //  - night shifts cannot appear on consecutive days:    Sequence(X, 2, 0, 1, {N})
    Factory::seqMDD(mdd->getSpec(), vars, 2, 0, 1, {3});
    
    //  - between 2 and 4 evening/night shifts every 7 days: Sequence(X, 7, 2, 4, {E, N})
    Factory::seqMDD(mdd->getSpec(), vars, 7, 2, 4, {2,3});
    
    //  - at most 6 work shifts every 7 days:                Sequence(X, 7, 0, 6, {D, E, N})
    Factory::seqMDD(mdd->getSpec(), vars, 7, 0, 6, {1,2,3});
    
    cp->post(mdd);
  }
  else if (mode == 2) {

    cout << "seqMDD2 encoding" << endl;
  
    auto mdd = new MDDRelax(cp,relaxSize);
  
    //  - at least 4 off-days every 14 days:                 Sequence(X, 14, 4, 14, {O})
    Factory::seqMDD2(mdd->getSpec(), vars, 14, 4, 14, {0});

    //  - at least 20 work shifts every 28 days:             Sequence(X, 28, 20, 28, {D, E, N})
    Factory::seqMDD2(mdd->getSpec(), vars, 28, 20, 28, {1,2,3});
  
    //  - between 1 and 4 night shifts every 14 days:        Sequence(X, 14, 1, 4, {N})
    Factory::seqMDD2(mdd->getSpec(), vars, 14, 1, 4, {3});
    
    //  - between 4 and 8 evening shifts every 14 days:      Sequence(X, 14, 4, 8, {E})
    Factory::seqMDD2(mdd->getSpec(), vars, 14, 4, 8, {2});
    
    //  - night shifts cannot appear on consecutive days:    Sequence(X, 2, 0, 1, {N})
    Factory::seqMDD2(mdd->getSpec(), vars, 2, 0, 1, {3});
    
    //  - between 2 and 4 evening/night shifts every 7 days: Sequence(X, 7, 2, 4, {E, N})
    Factory::seqMDD2(mdd->getSpec(), vars, 7, 2, 4, {2,3});
    
    //  - at most 6 work shifts every 7 days:                Sequence(X, 7, 0, 6, {D, E, N})
    Factory::seqMDD2(mdd->getSpec(), vars, 7, 0, 6, {1,2,3});
    
    cp->post(mdd);
  }
  else if (mode == 3) {

    cout << "seqMDD3 encoding" << endl;

    auto mdd = new MDDRelax(cp,relaxSize);
  
    //  - at least 4 off-days every 14 days:                 Sequence(X, 14, 4, 14, {O})
    Factory::seqMDD3(mdd->getSpec(), vars, 14, 4, 14, {0});

    //  - at least 20 work shifts every 28 days:             Sequence(X, 28, 20, 28, {D, E, N})
    Factory::seqMDD3(mdd->getSpec(), vars, 28, 20, 28, {1,2,3});
  
    //  - between 1 and 4 night shifts every 14 days:        Sequence(X, 14, 1, 4, {N})
    Factory::seqMDD3(mdd->getSpec(), vars, 14, 1, 4, {3});
    
    //  - between 4 and 8 evening shifts every 14 days:      Sequence(X, 14, 4, 8, {E})
    Factory::seqMDD3(mdd->getSpec(), vars, 14, 4, 8, {2});
    
    //  - night shifts cannot appear on consecutive days:    Sequence(X, 2, 0, 1, {N})
    Factory::seqMDD3(mdd->getSpec(), vars, 2, 0, 1, {3});
    
    //  - between 2 and 4 evening/night shifts every 7 days: Sequence(X, 7, 2, 4, {E, N})
    Factory::seqMDD3(mdd->getSpec(), vars, 7, 2, 4, {2,3});
    
    //  - at most 6 work shifts every 7 days:                Sequence(X, 7, 0, 6, {D, E, N})
    Factory::seqMDD3(mdd->getSpec(), vars, 7, 0, 6, {1,2,3});
    
    cp->post(mdd);
  }
  
  DFSearch search(cp,[=]() {
      unsigned i;
      for(i=0u;i< vars.size();i++)
         if (vars[i]->size() > 1)
            break;
      auto x = (i < vars.size()) ? vars[i] : nullptr;

      // int depth = 0;
      // for(auto i=0u;i < vars.size();i++) 
      // 	depth += vars[i]->size() == 1;

      // auto x = selectMin(vars,
      // 			 [](const auto& x) { return x->size() > 1;},
      // 			 [](const auto& x) { return x->size();});
     
      if (x) {
	int c = x->min();
	
	return  [=] {
                   //cout << tab(depth) << "?x(" << i << ") == " << c << " " <<  x << endl;
		  cp->post(x == c);
		  //cout << tab(depth) << "!x(" << i << ") == " << c << " " <<  x << endl;
		}
	  | [=] {
               //cout << tab(depth) << "?x(" << i << ") != " << c << " " <<  x << endl;
	      cp->post(x != c);
	      //cout << tab(depth) << "!x(" << i << ") != " << c << " " <<  x << endl;
	    };
      } else return Branches({});
    });
  
  int cnt = 0;
  search.onSolution([&vars,&cnt]() {
                       std::cout << "Assignment(" << cnt++ << "):" << " " << vars << std::endl;
    });

  auto start = RuntimeMonitor::cputime();

  auto stat = search.solve([](const SearchStatistics& stats) {
      //      return stats.numberOfSolutions() > INT_MAX;
      return stats.numberOfSolutions() > 0;
    }); 
  cout << stat << endl;

  auto end = RuntimeMonitor::cputime();
  extern int iterMDD;
  extern int nbCS;
  std::cout << "Time : " << RuntimeMonitor::milli(start,end) << '\n';
  std::cout << "I/C  : " << (double)iterMDD/stat.numberOfNodes() << '\n';
  std::cout << "#CS  : " << nbCS << '\n';
  std::cout << "#L   : " << mdd->nbLayers() << '\n'; 
}

int main(int argc,char* argv[])
{
   int width = (argc >= 2 && strncmp(argv[1],"-w",2)==0) ? atoi(argv[1]+2) : 1;
   int mode  = (argc >= 3 && strncmp(argv[2],"-m",2)==0) ? atoi(argv[2]+2) : 1;

   // mode: 0 (cumulative sums encoding), >=1 (MDD)
   
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
