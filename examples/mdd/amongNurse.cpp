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

template <class Vec>
Vec all(CPSolver::Ptr cp,const set<int>& over,const Vec& t)
{
   Vec res(over.size(),t.get_allocator()); //= Factory::intVarArray(cp, (int) over.size());
   int i = 0;
   for(auto e : over)
      res[i++] = t[e];
   return res;
}

template <class Vec>
void addCumulSeq(CPSolver::Ptr cp, const Vec& vars, int N, int L, int U, const std::set<int> S) {

  int H = (int)vars.size();
  
  auto cumul = Factory::intVarArray(cp, H+1, 0, H); 
  cp->post(cumul[0] == 0);
    
  // std::vector<var<bool>::Ptr> boolVar(H);
  // for (int i=0; i<H; i++) {
  //   boolVar[i] = isMember(vars[i], S);
  // }
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
   * Nurse scheduling problem from [Hoda et al. CP 2010] [Van Hoeve et al. Constraints 2009].
   * Determine the work schedule for a single nurse over time horizon {1..H}.
   * Class C-I:
   *   - work at most 6 out of each 8 consecutive days
   *   - work at least 22 out of each 30 consecutive days
   * Class C-II:
   *   - work at most 6 out of each 9 consecutive days
   *   - work at least 20 out of each 30 consecutive days
   * Class C-III:
   *   - work at most 7 out of each 9 consecutive days
   *   - work at least 22 out of each 30 consecutive days
   * In each class, we need to work between 4 and 5 days during each calendar week.
   * The planning horizon H ranges from 40 to 80 days.
   ***/
  
  int H = 40; // time horizon (number of workdays)
  
  //auto vars = Factory::intVarArray(cp, H, 0, 1); // vars[i]=1 means that the nurse works on day i
  auto vars = Factory::boolVarArray(cp, H); // vars[i]=1 means that the nurse works on day i

  std::set<int> workDay = {1};

  int L1 = 0;
  int U1 = 6;
  int N1 = 8;

  int L2 = 22;
  int U2 = 30;
  int N2 = 30;

  int L3 = 4;
  int U3 = 5;
  int N3 = 7;
  
  auto mdd = new MDDRelax(cp,relaxSize);

  if (mode == 0) {
    cout << "domain encoding of cumulative sums" << endl;
    
    // constraint type 1
    auto cumul = Factory::intVarArray(cp, H+1, 0, H); // cumulative sum: cumul[i+1] = vars[0] + ... + vars[i]
    cp->post(cumul[0] == 0);

    // new ternary equality cumul[i+1] = cumul[i] + vars[i]
    for (int i=0; i<H; i++) {
      cp->post(equal(cumul[i+1], cumul[i], vars[i]));
    }
    
    for (int i=0; i<H-N1+1; i++) {
      cp->post(cumul[i+N1] <= cumul[i] + U1);
      cp->post(cumul[i+N1] >= cumul[i] + L1);
    }

    // constraint type 2

    auto cumul2 = Factory::intVarArray(cp, H+1, 0, H); // cumulative sum: cumul[i+1] = vars[0] + ... + vars[i]
    cp->post(cumul2[0] == 0);

    // new ternary equality cumul[i+1] = cumul[i] + vars[i]
    for (int i=0; i<H; i++) {
      cp->post(equal(cumul2[i+1], cumul2[i], vars[i]));
    }
        
    for (int i=0; i<H-N2+1; i++) {
      cp->post(cumul2[i+N2] <= cumul2[i] + U2);
      cp->post(cumul2[i+N2] >= cumul2[i] + L2);
    }

    // constraint type 3
    for (int i=0; i<H/N3; i++) {
      cout << "Sum for week " << i << ": ";
      if (7*i+7<H) {
	set<int> weekVars;
	for (int j=7*i; j<7*i+7; j++) {
	  weekVars.insert(j);    
	  cout << j << " ";
	}
	cout << endl;
	
	auto adv = all(cp, weekVars,vars);
	// post as simple sums (baseline model in [Van Hoeve 2009])
	cp->post(sum(adv) >= L3);
	cp->post(sum(adv) <= U3);
      }
    }    
  }
  else if (mode == 1) {
    cout << "Among MDD encoding" << endl;

    // constraint 1
    cout << "Constraint type 1" << endl; 
    for (int i=0; i<H-N1+1; i++) {
      cout << "among " << i << ": ";
      set<int> amongVars;
      for (int j=i; j<i+N1; j++) {
	amongVars.insert(j);
	cout << j << " ";
      }
      cout << endl;
      
      auto adv = all(cp, amongVars, vars);
      Factory::amongMDD(mdd->getSpec(), adv, L1, U1, workDay);
    }
    
    // constraint 2
    cout << "Constraint type 2" << endl;
    for (int i=0; i<H-N2+1; i++) {
      cout << "among " << i << ": ";
      set<int> amongVars;
      for (int j=i; j<i+N2; j++) {
	amongVars.insert(j);    
	cout << j << " ";
      }
      cout << endl;
      
      auto adv = all(cp, amongVars, vars);
      Factory::amongMDD(mdd->getSpec(), adv, L2, U2, workDay);
    }
  
    // constraint 3
    cout << "Constraint type 3" << endl;
    for (int i=0; i<H/N3; i++) {
      cout << "Among for week " << i << ": ";
      if (7*i+7<H) {
	set<int> amongVars;
	for (int j=7*i; j<7*i+7; j++) {
	  amongVars.insert(j);    
	  cout << j << " ";
	}
	cout << endl;
	
	auto adv = all(cp, amongVars,vars);
	Factory::amongMDD(mdd->getSpec(), adv, L3, U3, workDay);
      }
    }
    cp->post(mdd);
  }
  else if (mode == 2) {
    cout << "Sequence MDD2 encoding" << endl;

    // constraint 1
    cout << "Sequence(vars," << N1 << "," << L1 << "," << U1 << ",{1})" << std::endl;
    Factory::seqMDD2(mdd->getSpec(), vars, N1, L1, U1, workDay);

    // constraint 2
    cout << "Sequence(vars," << N2 << "," << L2 << "," << U2 << ",{1})" << std::endl;
    Factory::seqMDD2(mdd->getSpec(), vars, N2, L2, U2, workDay);

    // constraint 3
    cout << "Constraint type 3" << endl;
    for (int i=0; i<H/N3; i++) {
      cout << "Among for week " << i << ": ";
      if (7*i+7<H) {
	set<int> amongVars;
	for (int j=7*i; j<7*i+7; j++) {
	  amongVars.insert(j);    
	  cout << j << " ";
	}
	cout << endl;
	
	auto adv = all(cp,amongVars,vars);
	Factory::amongMDD(mdd->getSpec(), adv, L3, U3, workDay);
      }
    }
    cp->post(mdd);
  }
  else if (mode == 3) {
    cout << "Sequence MDD3 encoding" << endl;

    // constraint 1
    cout << "Sequence(vars," << N1 << "," << L1 << "," << U1 << ",{1})" << std::endl;
    Factory::seqMDD3(mdd->getSpec(), vars, N1, L1, U1, workDay);

    // constraint 2
    cout << "Sequence(vars," << N2 << "," << L2 << "," << U2 << ",{1})" << std::endl;
    Factory::seqMDD3(mdd->getSpec(), vars, N2, L2, U2, workDay);

    // constraint 3
    cout << "Constraint type 3" << endl;
    for (int i=0; i<H/N3; i++) {
      cout << "Among for week " << i << ": ";
      if (7*i+7<H) {
	set<int> amongVars;
	for (int j=7*i; j<7*i+7; j++) {
	  amongVars.insert(j);    
	  cout << j << " ";
	}
	cout << endl;
	
	auto adv = all(cp, amongVars, vars);
	Factory::amongMDD(mdd->getSpec(), adv, L3, U3, workDay);
      }
    }
    cp->post(mdd);
    mdd->saveGraph();
  }
  else if (mode == 4) {
    cout << "Cumulative Sums with isMember encoding" << endl;

    // constraint 1
    cout << "Sequence(vars," << N1 << "," << L1 << "," << U1 << ",{1})" << std::endl;
    addCumulSeq(cp, vars, N1, L1, U1, workDay);
    
    // constraint 2
    cout << "Sequence(vars," << N2 << "," << L2 << "," << U2 << ",{1})" << std::endl;
    addCumulSeq(cp, vars, N2, L2, U2, workDay);


    // constraint type 3
    for (int i=0; i<H/N3; i++) {
      cout << "Sum for week " << i << ": ";
      if (7*i+7<H) {
	set<int> weekVars;
	for (int j=7*i; j<7*i+7; j++) {
	  weekVars.insert(j);    
	  cout << j << " ";
	}
	cout << endl;
	
	auto adv = all(cp, weekVars, vars);
	// post as simple sums (baseline model in [Van Hoeve 2009])
	cp->post(sum(adv) >= L3);
	cp->post(sum(adv) <= U3);
      }
    }    
  }
  
  
  DFSearch search(cp,[=]() {
      unsigned i = 0u;
      for(i=0u;i < vars.size();i++)
	if (vars[i]->size()> 1) break;
      auto x = i< vars.size() ? vars[i] : nullptr;

      // auto x = selectMin(vars,
      // 			 [](const auto& x) { return x->size() > 1;},
      // 			 [](const auto& x) { return x->size();});
      
      if (x) {
	bool c = x->min();
	
	return  [=] {
	  cp->post(x == c);}
	| [=] {
	  cp->post(x != c);};
      } else return Branches({});
    });

  int cnt = 0;
  search.onSolution([&cnt]() {
      cnt++;
      std::cout << "\rNumber of solutions:" << cnt << std::flush;
      //std::cout << "Assignment:" << vars << std::endl;
    });

  auto start = RuntimeMonitor::cputime();

  auto stat = search.solve([](const SearchStatistics& stats) {
      return stats.numberOfSolutions() > INT_MAX;
      //return stats.numberOfSolutions() > 0;
    }); 
  cout << stat << endl;
  
  auto end = RuntimeMonitor::cputime();
  std::cout << "Time : " << RuntimeMonitor::milli(start,end) << std::endl;
}

int main(int argc,char* argv[])
{
   int width = (argc >= 2 && strncmp(argv[1],"-w",2)==0) ? atoi(argv[1]+2) : 1;
   int mode  = (argc >= 3 && strncmp(argv[2],"-m",2)==0) ? atoi(argv[2]+2) : 1;

   // mode: 0 (Cumulative sums),  1 (Among MDD), 2 (Sequence MDD)
   // mode: 3 (Cumulative Sums with isMember constraint)
   
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
