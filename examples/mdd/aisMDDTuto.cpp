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

   MDDCstrDesc::Ptr absDiffMDD(MDD::Ptr m, const Factory::Veci& vars) {
     MDDSpec& mdd = m->getSpec();
     assert(vars.size()==3);
     
     // Filtering rules based the following constraint:  |vars[0]-vars[1]| = vars[2]
     // referred to below as |x-y| = z.
     auto d = mdd.makeConstraintDescriptor(vars,"absDiffMDD");
     const auto udom  = domRange(vars);
     const int minDom = udom.first;
    
    const auto xSome  = mdd.downBSState(d,udom.second - udom.first + 1,0,MinFun);
    const auto ySome  = mdd.downBSState(d,udom.second - udom.first + 1,0,MinFun);
    const auto N      = mdd.downIntState(d,0,INT_MAX,MinFun);        // layer index
    
    const auto ySomeUp = mdd.upBSState(d,udom.second - udom.first + 1,0,MinFun);
    const auto zSomeUp = mdd.upBSState(d,udom.second - udom.first + 1,0,MinFun);
    const auto NUp     = mdd.upIntState(d,0,INT_MAX,MinFun);        // layer index
    
    mdd.transitionDown(d,xSome,{xSome,N},{},[=](auto& out,const auto& p,auto,const auto& val)  noexcept {
      out[xSome] = p.down[xSome]; // another syntax      
      if (p.down[N]==0) {
        auto sv = out[xSome];
        for(auto v : val)
          sv.set(v - minDom);
      }
    });
    mdd.transitionDown(d,ySome,{ySome,N},{},[=](auto& out,const auto& p,auto,const auto& val)  noexcept {
      out[ySome] = p.down[ySome];
      if (p.down[N]==1) {
        auto sv = out[ySome];
        for(auto v : val)
          sv.set(v - minDom);
      }
    });

    mdd.transitionDown(d,N,{N},{},[N](auto& out,const auto& p,auto,const auto&) noexcept     {
       out[N]   = p.down[N]+1;
    });
    mdd.transitionUp(d,NUp,{NUp},{},[NUp](auto& out,const auto& c,auto,const auto&) noexcept {
       out[NUp] = c.up[NUp]+1;
    });

    mdd.transitionUp(d,ySomeUp,{ySomeUp,NUp},{},[=](auto& out,const auto& c,auto, const auto& val) noexcept {
      out[ySomeUp] = c.up[ySomeUp];
      if (c.up[NUp]==1) {
        auto sv(out[ySomeUp]);
        for(auto v : val)
          sv.set(v - minDom);                                 
      }
    });
    mdd.transitionUp(d,zSomeUp,{zSomeUp,NUp},{},[=](auto& out,const auto& c,auto, const auto& val) noexcept  {
       out[zSomeUp] = c.up[zSomeUp];
       if (c.up[NUp]==0) {
         auto sv = out[zSomeUp];
         for(auto v : val)
           sv.set(v - minDom);                                 
       }
    });

    mdd.arcExist(d,[=](const auto& p,const auto& c,var<int>::Ptr, const auto& val)  noexcept {
       switch(p.down[N]) {
          case 0: { // filter x variable
             MDDBSValue yVals = c.up[ySomeUp],zVals = c.up[zSomeUp];
             for(auto yofs : yVals) {
               const auto i = yofs + minDom;
               if (i == val) continue;
               const int zval = std::abs(val-i);
               if (zval >= udom.first && zval <= udom.second && zVals.getBit(zval))
                 return true;
             }    
             return false;
          }break;
          case 1: { // filter y variable
             MDDBSValue xVals = p.down[xSome],zVals = c.up[zSomeUp];
             for(auto xofs : xVals) {
                const auto i = xofs + minDom;
                if (i == val) continue;
                const int zval = std::abs(i-val);
                if (zval >= udom.first && zval <= udom.second && zVals.getBit(zval))
                   return true;
             }    
             return false;
          }break;
          case 2: { // filter z variable
             MDDBSValue xVals = p.down[xSome],yVals = p.down[ySome];
             for(const auto xofs : xVals) {
                const auto i = xofs + minDom;
                if (val==0) continue;
                const int yval1 = i-val,yval2 = i+val;
                if ((yval1 >= udom.first && yval1 <= udom.second && yVals.getBit(yval1)) ||
                    (yval2 >= udom.first && yval2 <= udom.second && yVals.getBit(yval2)))
                   return true;
             }    
             return false;
          }break;
          default: return true;
       }
       return true;
    });
      
    mdd.addRelaxationDown(d,xSome,[xSome](auto& out,const auto& l,const auto& r)  noexcept     {
       out[xSome].setBinOR(l[xSome],r[xSome]);
    });
    mdd.addRelaxationDown(d,ySome,[ySome](auto& out,const auto& l,const auto& r)  noexcept     {
       out[ySome].setBinOR(l[ySome],r[ySome]);
    });
    mdd.addRelaxationUp(d,ySomeUp,[ySomeUp](auto& out,const auto& l,const auto& r)  noexcept     {
       out[ySomeUp].setBinOR(l[ySomeUp],r[ySomeUp]);
    });
    mdd.addRelaxationUp(d,zSomeUp,[zSomeUp](auto& out,const auto& l,const auto& r)  noexcept      {
       out[zSomeUp].setBinOR(l[zSomeUp],r[zSomeUp]);
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
   int maxReboot    = (argc >= 5 && strncmp(argv[3],"-r",2)==0) ? atoi(argv[4]+2) : 0;

   cout << "N = " << N << endl;   
   cout << "width = " << width << endl;   
   cout << "max reboot distance = " << maxReboot << endl;

   CPSolver::Ptr cp  = Factory::makeSolver();

   auto v = Factory::intVarArray(cp, 2*N-1, 0, N-1);

   set<int> xIdx = filter(range(0,2*N-2),[](int i) {return i==0 || i%2!=0;});
   set<int> yIdx = filter(range(2,2*N-2),[](int i) {return i%2==0;});
   auto x = all(cp, xIdx, [&v](int i) {return v[i];});
   auto y = all(cp, yIdx, [&v](int i) {return v[i];});

   for (auto i=0u; i<y.size(); i++) 
      cp->post(y[i] != 0);
   auto mdd = Factory::makeMDDRelax(cp,width,maxReboot);
   mdd->post(Factory::allDiffMDD(x));
   mdd->post(Factory::allDiffMDD(y));
   for(int i=0; i < N-1;i++)
      mdd->post(Factory::absDiffMDD(mdd,{y[i],x[i+1],x[i]}));
   cp->post(mdd);

   DFSearch search(cp,[=]() { 
      return indomain_min(cp,selectFirst(x,[](const auto& x) { return x->size() > 1;}));
   });
   SearchStatistics stat = search.solve([](const SearchStatistics& stats) {
      return stats.numberOfSolutions() > INT_MAX;
   });
   cout << stat << "\n";
   return 0;
}
