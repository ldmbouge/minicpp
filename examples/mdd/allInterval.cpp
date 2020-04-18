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
#include "mddConstraints.hpp"
#include "RuntimeMonitor.hpp"
#include "table.hpp"
#include "allInterval.hpp"


using namespace std;
using namespace Factory;

/*** Domain (bounds) propagation for AbsDiff constraint ***/

bool propagateExpression(var<int>::Ptr _z,var<int>::Ptr _x,var<int>::Ptr _y,
			 int &v1min, int &v1max, int &v3min, int &v3max ) {
  // Apply interval propagation for expression [z] = [Abs[ [[x] - [y]] ]] :
  // First propagate x, y, z bounds 'up' through to intersect [z] == [v3]
  // Then propagate the intervals 'down' back to x, y, z
  
  // std::cout << "enter propagateExpression(" << _z << ", " << _x << ", " << _y << ")" << std::endl;

  
     // Up-propagate:
     v1min = _x->min() - _y->max();
     v1max = _x->max() - _y->min();
     int v2min = 0;
     if (!((v1max >= 0) && (v1min<=0)))
       v2min = std::min(std::abs(v1min), std::abs(v1max));
     int v2max = std::max(std::abs(v1min), std::abs(v1max));
     v3min = std::max(_z->min(), v2min);
     v3max = std::min(_z->max(), v2max);

     // Down-propagate for v1, v2, v3 (their bounds will be used for x, y, z):
     v2min = std::max(v2min, v3min);
     v2max = std::min(v2max, v3max);
     v1min = std::max(v1min, -v2max);
     v1max = std::min(v1max, v2max);

     if ((v1max < v1min) || (v2max < v2min) || (v3max < v3min)) {
       std::cout << "propagateExpression(" << _z << ", " << _x << ", " << _y << "): "
		 << _z << " = |" << _x << "-" << _y << "| returns false because ";
       if (v1max < v1min) std::cout << "(v1max < v1min)";
       if (v2max < v2min) std::cout << "(v2max < v2min)";
       if (v3max < v2min) std::cout << "(v3max < v3min)";
       std::cout << std::endl;
       return false;
     }
     return true;
}
void EQAbsDiffBC::post()
{
   // z == |x - y|
   if (_x->isBound() && _y->isBound()) {
     _z->assign(std::abs(_x->min()-_y->min()));
   }
   else {

     int v1min, v1max, v3min, v3max;
     bool check = propagateExpression(_x, _y, _z, v1min, v1max, v3min, v3max);
     std::cout << "v1min = " << v1min << ", v1max = " << v1max << "v3min = " << v3min << ", v3max = " << v3max << std::endl;
     if (!check) { throw Failure; }

     _z->updateBounds(std::max(_z->min(),v3min), std::min(_z->max(),v3max));
     _x->updateBounds(std::max(_x->min(),_y->min()+v1min), std::min(_x->max(),_y->max()+v1max));
     _y->updateBounds(std::max(_y->min(),_x->min()-v1max), std::min(_y->max(),_x->max()-v1min));
      
     _x->whenBoundsChange([this] {
	 // int v1min, v1max, v3min, v3max;
	 // bool check = propagateExpression(_x, _y, _z, v1min, v1max, v3min, v3max);
	 // std::cout << "v1min = " << v1min << ", v1max = " << v1max << ", v3min = " << v3min << ", v3max = " << v3max << std::endl;
	 // if (!check) { throw Failure; }

	 _z->updateBounds(std::max(_z->min(),v3min), std::min(_z->max(),v3max));
	 _y->updateBounds(std::max(_y->min(),_x->min()-v1max), std::min(_y->max(),_x->max()-v1min));

       });
      
      _y->whenBoundsChange([this] {
	 int v1min, v1max, v3min, v3max;
	 bool check = propagateExpression(_x, _y, _z, v1min, v1max, v3min, v3max);
	  _z->updateBounds(std::max(_z->min(),v3min), std::min(_z->max(),v3max));
	  _x->updateBounds(std::max(_x->min(),_y->min()+v1min), std::min(_x->max(),_y->max()+v1max));
	});

      _z->whenBoundsChange([this] {
	 int v1min, v1max, v3min, v3max;
	 bool check = propagateExpression(_x, _y, _z, v1min, v1max, v3min, v3max);
	  _x->updateBounds(std::max(_x->min(),_y->min()+v1min), std::min(_x->max(),_y->max()+v1max));
	  _y->updateBounds(std::max(_y->min(),_x->min()-v1max), std::min(_y->max(),_x->max()-v1min));
	});
   }
}
/***/


Veci all(CPSolver::Ptr cp,const set<int>& over, std::function<var<int>::Ptr(int)> clo)
{
   auto res = Factory::intVarArray(cp, (int) over.size());
   int i = 0;
   for(auto e : over){
      res[i++] = clo(e);
   }
   return res;
}



int main(int argc,char* argv[])
{

   int N     = (argc >= 2 && strncmp(argv[1],"-n",2)==0) ? atoi(argv[1]+2) : 8;
   int width = (argc >= 3 && strncmp(argv[2],"-w",2)==0) ? atoi(argv[2]+2) : 1;
   int mode  = (argc >= 4 && strncmp(argv[3],"-m",2)==0) ? atoi(argv[3]+2) : 0;

   cout << "N = " << N << endl;   
   cout << "width = " << width << endl;   
   cout << "mode = " << mode << endl;

   auto start = RuntimeMonitor::cputime();

   CPSolver::Ptr cp  = Factory::makeSolver();
   // auto xVars = Factory::intVarArray(cp, N, 0, N-1);
   // auto yVars = Factory::intVarArray(cp, N-1, 1, N-1);

   auto vars = Factory::intVarArray(cp, 2*N-1, 0, N-1);
   // vars[0] = x[0]
   // vars[1] = x[1]
   // vars[2] = y[1]
   // vars[3] = x[2]
   // vars[4] = y[2]
   // ...
   // vars[i] = x[ ceil(i/2) ] if i is odd
   // vars[i] = y[ i/2 ]       if i is even

   set<int> xVarsIdx;
   set<int> yVarsIdx;
   xVarsIdx.insert(0);
   for (int i=1; i<2*N-1; i++) {
     if ( i%2==0 ) {
       cp->post(vars[i] != 0);
       yVarsIdx.insert(i);
     }
     else {
       xVarsIdx.insert(i);
     }
   }
   auto xVars = all(cp, xVarsIdx, [&vars](int i) {return vars[i];});
   auto yVars = all(cp, yVarsIdx, [&vars](int i) {return vars[i];});


   std::cout << "x = " << xVars << endl;
   std::cout << "y = " << yVars << endl;
   
   
   auto mdd = new MDDRelax(cp,width);

   if (mode == 0) {
     cout << "domain encoding with equalAbsDiff constraint" << endl;
     cp->post(Factory::allDifferentAC(xVars));
     cp->post(Factory::allDifferentAC(yVars));
     for (int i=0; i<N-1; i++) {
       cp->post(equalAbsDiff(yVars[i], xVars[i+1], xVars[i]));
     }
   }
   if ((mode == 1) || (mode == 3)) {
     cout << "domain encoding with AbsDiff-Table constraint" << endl;
     cp->post(Factory::allDifferentAC(xVars));
     cp->post(Factory::allDifferentAC(yVars));

     std::vector<std::vector<int>> table;
     for (int i=0; i<N-1; i++) {
       for (int j=i+1; j<N; j++) {
	 std::vector<int> t1;
	 t1.push_back(i);
	 t1.push_back(j);
	 t1.push_back( std::abs(i-j) );	   
	 table.emplace_back(t1);
	 std::vector<int> t2;
	 t2.push_back(j);
	 t2.push_back(i);
	 t2.push_back( std::abs(i-j) );
	 table.emplace_back(t2);
       }
     }
     std::cout << table << std::endl;
     auto tmpFirst = all(cp, {0,1,2}, [&vars](int i) {return vars[i];});     
     cp->post(Factory::table(tmpFirst, table));
     for (int i=1; i<N-1; i++) {
       std::set<int> tmpVarsIdx;
       tmpVarsIdx.insert(2*i-1);
       tmpVarsIdx.insert(2*i+1);
       tmpVarsIdx.insert(2*i+2);       
       auto tmpVars = all(cp, tmpVarsIdx, [&vars](int i) {return vars[i];});
       cp->post(Factory::table(tmpVars, table));       
     }
   }
   if ((mode == 2) || (mode == 3)) {
     cout << "MDD encoding" << endl;     
     auto tmpFirst = all(cp, {0,1,2}, [&vars](int i) {return vars[i];});     
     Factory::absDiffMDD(mdd->getSpec(),tmpFirst);
     for (int i=1; i<N-1; i++) {
       std::set<int> tmpVarsIdx;
       tmpVarsIdx.insert(2*i-1);
       tmpVarsIdx.insert(2*i+1);
       tmpVarsIdx.insert(2*i+2);       
       auto tmpVars = all(cp, tmpVarsIdx, [&vars](int i) {return vars[i];});
       Factory::absDiffMDD(mdd->getSpec(),tmpVars);
     }
     Factory::allDiffMDD(mdd->getSpec(),xVars);
     Factory::allDiffMDD(mdd->getSpec(),yVars);
     cp->post(mdd);
     //mdd->saveGraph();
   }
   if ((mode < 0) || (mode > 3)) {
     cout << "Exit: specify a mode in {0,1,2,3}:" << endl;
     cout << "  0: domain encoding using AbsDiff" << endl;
     cout << "  1: domain encoding using Table" << endl;
     cout << "  2: MDD encoding" << endl;
     cout << "  3: domain (table) + MDD encoding" << endl;
     exit(1);
   }

   DFSearch search(cp,[=]() {

      //  // Lex order
      // unsigned i = 0u;
      // for(i=0u;i < xVars.size();i++)
      // 	if (xVars[i]->size()> 1) break;
      // auto x = i< xVars.size() ? xVars[i] : nullptr;

       // Fail first
       auto x = selectMin(xVars,
                         [](const auto& x) { return x->size() > 1;},
                         [](const auto& x) { return x->size();});

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
      

   int cnt = 0;
   search.onSolution([&cnt,xVars,yVars]() {
       cnt++;
       // std::cout << "\rNumber of solutions:" << cnt << std::flush;
        	 std::cout << "x = " << xVars << endl;
        	 std::cout << "y = " << yVars << endl;
     });

      
   // auto stat = search.solve([](const SearchStatistics& stats) {
   //    return stats.numberOfSolutions() > 0;
   // });
   auto stat = search.solve();

   auto end = RuntimeMonitor::cputime();
   cout << stat << endl;
   std::cout << "Time : " << RuntimeMonitor::milli(start,end) << std::endl;
      
   cp.dealloc();
   return 0;
}
