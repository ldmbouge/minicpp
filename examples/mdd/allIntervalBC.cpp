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
#include "range.hpp"
#include "allInterval.hpp"


using namespace std;
using namespace Factory;


namespace Factory {
   MDDCstrDesc::Ptr absDiffMDD(MDD::Ptr m,std::initializer_list<var<int>::Ptr> vars) {
      CPSolver::Ptr cp = (*vars.begin())->getSolver();
      auto theVars = Factory::intVarArray(cp,vars.size(),[&vars](int i) {
         return std::data(vars)[i];
      });
      return absDiffMDD(m,theVars);
   }
   MDDCstrDesc::Ptr absDiffMDD(MDD::Ptr m,const Factory::Veci& vars)
   {
      MDDSpec& mdd = m->getSpec();
      assert(vars.size()==3);    
      // Filtering rules based the following constraint:
      //   |vars[0]-vars[1]| = vars[2]
      // referred to below as |x-y| = z.
      auto d = mdd.makeConstraintDescriptor(vars,"absDiffMDD");    
      const auto xMin = mdd.downIntState(d,0,-INT_MAX,MinFun);
      const auto xMax = mdd.downIntState(d,0,INT_MAX,MaxFun);
      const auto yMin = mdd.downIntState(d,0,-INT_MAX,MinFun);
      const auto yMax = mdd.downIntState(d,0,INT_MAX,MaxFun);
      const auto yMinUp = mdd.upIntState(d,0,-INT_MAX,MinFun);
      const auto yMaxUp = mdd.upIntState(d,0,INT_MAX,MaxFun);
      const auto zMinUp = mdd.upIntState(d,0,-INT_MAX,MinFun);
      const auto zMaxUp = mdd.upIntState(d,0,INT_MAX,MaxFun);
      const auto N = mdd.downIntState(d,0,INT_MAX,MinFun); // layer index
      const auto NUp = mdd.upIntState(d,0,INT_MAX,MinFun); // layer index
      mdd.transitionDown(d,xMin,{xMin,N},{},[xMin,N](auto& out,const auto& parent,auto x, const auto& val) {
         out[xMin] = (parent.down[N]==0) ? val.min() : parent.down[xMin];
      });
      mdd.transitionDown(d,xMax,{xMax,N},{},[xMax,N](auto& out,const auto& parent,auto x, const auto& val) {
         out[xMax] = (parent.down[N]==0) ? val.max() : parent.down[xMax];
      });
      mdd.transitionDown(d,yMin,{yMin,N},{},[yMin,N](auto& out,const auto& parent,auto x, const auto& val) {
         out[yMin] = (parent.down[N]==1) ? val.min() : parent.down[yMin];
      });
      mdd.transitionDown(d,yMax,{yMax,N},{},[yMax,N](auto& out,const auto& parent,auto x, const auto& val) {
         out[yMax] = (parent.down[N]==1) ? val.max() : parent.down[yMax];
      });

      mdd.transitionDown(d,N,{N},{},[N](auto& out,const auto& parent,auto x,const auto& val)    { out[N] = parent.down[N]+1; });
      mdd.transitionUp(d,NUp,{NUp},{},[NUp](auto& out,const auto& child,auto x,const auto& val) { out[NUp] = child.up[NUp]+1; });

      mdd.transitionUp(d,yMinUp,{yMinUp,NUp},{},[yMinUp,NUp] (auto& out,const auto& child,auto x, const auto& val) {
         out[yMinUp] = (child.up[NUp]==1) ? val.min() : child.up[yMinUp];
      });
      mdd.transitionUp(d,yMaxUp,{yMaxUp,NUp},{},[yMaxUp,NUp](auto& out,const auto& child,auto x, const auto& val) {
         out[yMaxUp] = (child.up[NUp]==1) ? val.max() : child.up[yMaxUp];
      });
      mdd.transitionUp(d,zMinUp,{zMinUp,N},{},[zMinUp,N](auto& out,const auto& child,auto x, const auto& val) {
         out[zMinUp] = (child.up[N]==0) ? val.min() : child.up[zMinUp];
      });
      mdd.transitionUp(d,zMaxUp,{zMaxUp,N},{},[zMaxUp,N](auto& out,const auto& child,auto x, const auto& val) {         
         out[zMaxUp] = (child.up[N]==0) ? val.max() : child.up[zMaxUp];
      });

      mdd.arcExist(d,[=](const auto& parent,const auto& child,var<int>::Ptr var,const auto& val) {
        switch(parent.down[N]) {
          case 0: {
            // filter x variable
            const int lb = child.up[zMinUp], ub = child.up[zMaxUp];
            for (int y=child.up[yMinUp]; y<=child.up[yMaxUp]; y++) {
              if (y == val) continue;
              const int z=std::abs(val-y);
              if (lb <= z && z <= ub)
                return true;
            }
            return false;
          }break;
          case 1: {
            // filter y variable
            const int lb = child.up[zMinUp], ub = child.up[zMaxUp];
            for (int x=parent.down[xMin]; x<=parent.down[xMax]; x++) {
              if (x==val) continue;
              const int z = std::abs(x-val);
              if (lb <= z && z <= ub)
                return true;
            }
            return false;
          }break;
          case 2: {
            // filter z variable
            const int lb = parent.down[yMin], ub = parent.down[yMax];
            for (int x=parent.down[xMin]; x<=parent.down[xMax]; x++) {
              const int y0 = x - val;
              const int y1 = x + val;
              bool y0In = lb <= y0 && y0 <= ub;
              bool y1In = lb <= y1 && y1 <= ub;
              if (y0In || y1In) return true;
            }
            return false;
          }break;
          default: return true;
        }
      });
      return d;
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

template <class Container,typename UP>
std::set<typename Container::value_type> filter(const Container& c,const UP& pred)
{
   std::set<typename Container::value_type> r;
   for(auto e : c)
      if (pred(e))
         r.insert(e);
   return r;
}

int main(int argc,char* argv[])
{
   int N     = (argc >= 2 && strncmp(argv[1],"-n",2)==0) ? atoi(argv[1]+2) : 8;
   int width = (argc >= 3 && strncmp(argv[2],"-w",2)==0) ? atoi(argv[2]+2) : 1;
   int mode  = (argc >= 4 && strncmp(argv[3],"-m",2)==0) ? atoi(argv[3]+2) : 0;
   int maxRebootDistance  = (argc >= 5 && strncmp(argv[4],"-r",2)==0) ? atoi(argv[4]+2) : 0;
   int maxSplitIter = (argc >= 6 && strncmp(argv[5],"-i",2)==0) ? atoi(argv[5]+2) : INT_MAX;

   cout << "N = " << N << endl;   
   cout << "width = " << width << endl;   
   cout << "mode = " << mode << endl;
   cout << "max reboot distance = " << maxRebootDistance << endl;
   cout << "max split iterations = " << maxSplitIter << endl;

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

   set<int> xVarsIdx = filter(range(0,2*N-2),[](int i) {return i==0 || i%2!=0;});
   set<int> yVarsIdx = filter(range(2,2*N-2),[](int i) {return i%2==0;});

   for (int i=2; i<2*N-1; i+=2) 
      cp->post(vars[i] != 0);

   auto xVars = all(cp, xVarsIdx, [&vars](int i) {return vars[i];});
   auto yVars = all(cp, yVarsIdx, [&vars](int i) {return vars[i];});

   std::cout << "x = " << xVars << endl;
   std::cout << "y = " << yVars << endl;
      
   auto mdd = Factory::makeMDDRelax(cp,width,maxRebootDistance,maxSplitIter);

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
            table.emplace_back(std::vector<int> {i,j,std::abs(i-j)});           
            table.emplace_back(std::vector<int> {j,i,std::abs(i-j)});
         }
      }
      std::cout << table << std::endl;
      auto tmpFirst = all(cp, {0,1,2}, [&vars](int i) {return vars[i];});     
      cp->post(Factory::table(tmpFirst, table));
      for (int i=1; i<N-1; i++) {
         std::set<int> tmpVarsIdx = {2*i-1,2*i+1,2*i+2};       
         auto tmpVars = all(cp, tmpVarsIdx, [&vars](int i) {return vars[i];});
         cp->post(Factory::table(tmpVars, table));       
      }
   }
   if ((mode == 2) || (mode == 3)) {
      cout << "MDD encoding" << endl;     
      mdd->post(Factory::absDiffMDD(mdd,{vars[0],vars[1],vars[2]}));
      for (int i=1; i<N-1; i++) 
         mdd->post(Factory::absDiffMDD(mdd,{vars[2*i-1],vars[2*i+1],vars[2*i+2]}));      
      mdd->post(Factory::allDiffMDD(mdd,xVars));
      mdd->post(Factory::allDiffMDD(mdd,yVars));
      cp->post(mdd);
      //mdd->saveGraph();
      cout << "For testing purposes: adding domain consistent AllDiffs MDD encoding" << endl;
      cp->post(Factory::allDifferentAC(xVars));
      cp->post(Factory::allDifferentAC(yVars));
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
      // Lex order
      auto x = selectFirst(xVars,[](const auto& x) { return x->size() > 1;});
      if (x) {	
         int c = x->min();          
         return  [=] {
            //std::string tabs(cp->getStateManager()->depth(),'\t');
            //cout << tabs <<  "->choose: " << x << " == " << c << endl; 
            cp->post(x == c);
            //cout << tabs << "<-choose: " << x << " == " << c << endl; 
         }     | [=] {
            //std::string tabs(cp->getStateManager()->depth(),'\t');
            //cout << tabs << "->choose: " << x << " != " << c << endl; 
            cp->post(x != c);
            //cout << tabs << "<-choose: " << x << " != " << c << endl; 
         };	
      } else return Branches({});
   });
      

   int cnt = 0;
   search.onSolution([&]() {
      cnt++;
      std::cout << "\rNumber of solutions:" << cnt; 
      //std::cout << "\nx = " << xVars << "\n";
      //std::cout << "y = " << yVars << endl;
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




// /*** Domain (bounds) propagation for AbsDiff constraint ***/
// bool propagateExpression(interval &_x, interval &_y, interval &_z, interval &v1, interval &v3) {

//   /*** 
      //    * Apply interval propagation to expression [z] = [Abs[ [[x] - [y]] ]] :
      //    * First propagate x, y, z bounds 'up' through to intersect [z] == [v3]
      //    * Then propagate the intervals 'down' back to x, y, z: this done in 
                                                                   //    * constraint, based on v1 and v3.
                                                                   //    ***/
  
                                                                   //   // Up-propagate:
                                                                   //   v1->min = _x->min - _y->max;
                                                                   //   v1->max = _x->max - _y->min;
                                                                   //   interval v2(0, 0);
                                                                   //   int v2min = 0;
                                                                   //   if (!((v1max >= 0) && (v1min<=0)))
                                                                   //     v2min = std::min(std::abs(v1min), std::abs(v1max));
                                                                   //   int v2max = std::max(std::abs(v1min), std::abs(v1max));
                                                                   //   v3min = std::max(_z->min(), v2min);
                                                                   //   v3max = std::min(_z->max(), v2max);
  
                                                                   //   // Down-propagate for v1, v2, v3 (their bounds will be used for x, y, z):
                                                                   //   v2min = std::max(v2min, v3min);
                                                                   //   v2max = std::min(v2max, v3max);
                                                                   //   v1min = std::max(v1min, -v2max);
                                                                   //   v1max = std::min(v1max, v2max);
  
                                                                   //   if ((v1max < v1min) || (v2max < v2min) || (v3max < v3min))
                                                                   //     return false;
                                                                   //   return true;
                                                                   // }
