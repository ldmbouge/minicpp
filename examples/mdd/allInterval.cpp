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


class interval {
public:
  int min, max;
  interval(int,int);
};

interval::interval(int _min, int _max) {
  min = _min;
  max = _max;
}


/*** Domain (bounds) propagation for AbsDiff constraint ***/
bool propagateExpression(const interval* _x, const interval* _y, const interval* _z, interval &v1, interval &v3) {

  /*** 
   * Apply interval propagation to expression [z] = [Abs[ [[x] - [y]] ]] :
   * First propagate x, y, z bounds 'up' to intersect [z] == [v3]
   * Then propagate the intervals 'down' back to x, y, z: this done in 
   * constraint, based on v1 and v3.
   ***/
  
  // Up-propagate:
  v1.min = _x->min - _y->max;
  v1.max = _x->max - _y->min;
  interval v2(-INT_MAX,INT_MAX);
  if (!((v1.max >= 0) && (v1.min<=0)))
    v2.min = std::min(std::abs(v1.min), std::abs(v1.max));
  v2.max = std::max(std::abs(v1.min), std::abs(v1.max));
  v3.min = std::max(_z->min, v2.min);
  v3.max = std::min(_z->max, v2.max);
  
  // Down-propagate for v1, v2, v3 (their bounds will be used for x, y, z):
  v2.min = std::max(v2.min, v3.min);
  v2.max = std::min(v2.max, v3.max);
  v1.min = std::max(v1.min, -(v2.max));
  v1.max = std::min(v1.max, v2.max);
  
  if ((v1.max < v1.min) || (v2.max < v2.min) || (v3.max < v3.min))
    return false;
  return true;
}
void EQAbsDiffBC::post()
{
   // z == |x - y|
   if (_x->isBound() && _y->isBound()) {
     _z->assign(std::abs(_x->min()-_y->min()));
   }
   else {
     interval v1(-INT_MAX,INT_MAX);
     interval v3(-INT_MAX,INT_MAX);
     interval X(_x->min(), _x->max());
     interval Y(_y->min(), _y->max());
     interval Z(_z->min(), _z->max());
     bool check = propagateExpression(&X, &Y, &Z, v1, v3);
     if (!check) { throw Failure; }

     _z->updateBounds(std::max(_z->min(),v3.min), std::min(_z->max(),v3.max));
     _x->updateBounds(std::max(_x->min(),_y->min()+v1.min), std::min(_x->max(),_y->max()+v1.max));
     _y->updateBounds(std::max(_y->min(),_x->min()-v1.max), std::min(_y->max(),_x->max()-v1.min));
      
     _x->whenBoundsChange([this] {
	 interval v1(-INT_MAX,INT_MAX);
	 interval v3(-INT_MAX,INT_MAX);
	 interval X(_x->min(), _x->max());
	 interval Y(_y->min(), _y->max());
	 interval Z(_z->min(), _z->max());
	 bool check = propagateExpression(&X, &Y, &Z, v1, v3);
	 if (!check) { throw Failure; }
     	 _z->updateBounds(std::max(_z->min(),v3.min), std::min(_z->max(),v3.max));
     	 _y->updateBounds(std::max(_y->min(),_x->min()-v1.max), std::min(_y->max(),_x->max()-v1.min));
       });
      
      _y->whenBoundsChange([this] {
	 interval v1(-INT_MAX,INT_MAX);
	 interval v3(-INT_MAX,INT_MAX);
	 interval X(_x->min(), _x->max());
	 interval Y(_y->min(), _y->max());
	 interval Z(_z->min(), _z->max());
	 bool check = propagateExpression(&X, &Y, &Z, v1, v3);
	 if (!check) { throw Failure; }
     	  _z->updateBounds(std::max(_z->min(),v3.min), std::min(_z->max(),v3.max));
     	  _x->updateBounds(std::max(_x->min(),_y->min()+v1.min), std::min(_x->max(),_y->max()+v1.max));
     	});

      // _z->whenBoundsChange([this] {
      // 	 interval v1(-INT_MAX,INT_MAX);
      // 	 interval v3(-INT_MAX,INT_MAX);
      // 	 interval X(_x->min(), _x->max());
      // 	 interval Y(_y->min(), _y->max());
      // 	 interval Z(_z->min(), _z->max());
      // 	 bool check = propagateExpression(&X, &Y, &Z, v1, v3);
      // 	 if (!check) { throw Failure; }
      // 	  _x->updateBounds(std::max(_x->min(),_y->min()+v1.min), std::min(_x->max(),_y->max()+v1.max));
      // 	  _y->updateBounds(std::max(_y->min(),_x->min()-v1.max), std::min(_y->max(),_x->max()-v1.min));
      // 	});

      // Applying domain consistency to x and y when Dom(z) changes seems to mimick the Puget&Regin approach.
      _z->whenDomainChange([this] {
	  for (int x=_x->min(); x<=_x->max(); x++) {
	    if (_x->contains(x)) {
	      bool support = false;
	      for (int y=_y->min(); y<=_y->max() && !support; y++) {
		if (_y->contains(y)) {
		  for (int z=_z->min(); z<=_z->max() && !support; z++) {
		    if (_z->contains(z) && (z==std::abs(x-y))){
		      support = true;
		      break;
		    }
		  }
		}
	      }
	      if (!support) { _x->remove(x); }
	    }
	  }
	  for (int y=_y->min(); y<=_y->max(); y++) {
	    if (_y->contains(y)) {
	      bool support = false;
	      for (int x=_x->min(); x<=_x->max() && !support; x++) {
		if (_x->contains(x)) {
		  for (int z=_z->min(); z<=_z->max() && !support; z++) {
		    if (_z->contains(z) && (z==std::abs(x-y))){
		      support = true;
		      break;
		    }
		  }
		}
	      }
	      if (!support) { _y->remove(y); }
	    }
	  }
     	});
   }
}
/***/

namespace Factory {

  void absDiffMDD(MDDSpec& mdd, const Factory::Veci& vars) {

    assert(vars.size()==3);
    
    // Filtering rules based the following constraint:  |vars[0]-vars[1]| = vars[2]
    // referred to below as |x-y| = z.
    mdd.append(vars);    
    auto d = mdd.makeConstraintDescriptor(vars,"absDiffMDD");

    auto udom  = domRange(vars);
    int minDom = udom.first;
    
    const int xSome   = mdd.addBSState(d,udom.second - udom.first + 1,0);
    const int ySome   = mdd.addBSState(d,udom.second - udom.first + 1,0);
    const int ySomeUp = mdd.addBSState(d,udom.second - udom.first + 1,0);
    const int zSomeUp = mdd.addBSState(d,udom.second - udom.first + 1,0);
    const int N       = mdd.addState(d,0,2,MinFun);        // layer index 
    
    mdd.transitionDown(xSome,{xSome},[xSome,N,minDom] (auto& out,const auto& p,auto x, const auto& val,bool up) {
	if (p.at(N)==0) {
	  out.setProp(xSome,p);
	  MDDBSValue sv(out.getBS(xSome));
	  for(auto v : val)
	    sv.set(v - minDom);
	}
	else
	  out.setProp(xSome,p);
      });
    mdd.transitionDown(ySome,{ySome},[ySome,N,minDom] (auto& out,const auto& p,auto x, const auto& val,bool up) {
	if (p.at(N)==1) {
	  out.setProp(ySome,p);
	  MDDBSValue sv(out.getBS(ySome));
	  for(auto v : val)
	    sv.set(v - minDom);
	}
	else
	  out.setProp(ySome,p);
      });

    mdd.transitionDown(N,{},[N](auto& out,const auto& p,auto x,const auto& val,bool up) { out.set(N,p.at(N)+1); });

    mdd.transitionUp(ySomeUp,{ySomeUp},[ySomeUp,N,minDom] (auto& out,const auto& c,auto x, const auto& val,bool up) {
	if (c.at(N)==2) {
	  out.setProp(ySomeUp,c);
	  MDDBSValue sv(out.getBS(ySomeUp));
	  for(auto v : val)
	    sv.set(v - minDom);                                 
	}
	else 
	  out.setProp(ySomeUp,c);
      });
    mdd.transitionUp(zSomeUp,{zSomeUp},[zSomeUp,N,minDom] (auto& out,const auto& c,auto x, const auto& val,bool up) {
	if (c.at(N)==3) {
	  out.setProp(zSomeUp,c);
	  MDDBSValue sv(out.getBS(zSomeUp));
	  for(auto v : val)
	    sv.set(v - minDom);                                 
	}
	else
	  out.setProp(zSomeUp,c);
      });

    mdd.arcExist(d,[=] (const auto& p,const auto& c,var<int>::Ptr var, const auto& val, bool up) -> bool {

	if (p.at(N)==2) {
	  // filter z variable
	  MDDBSValue xVals = p.getBS(xSome);
	  MDDBSValue yVals = p.getBS(ySome);
	  for (int i=udom.first; i<=udom.second; i++) {
	    const int xofs = i-minDom;
	    if (xVals.getBit(xofs)) {
	      int yval1 = i-val;
	      int yval2 = i+val;
	      if ((yval1 >= udom.first && yval1 <= udom.second && yVals.getBit(yval1) && yval1 != i) ||
		  (yval2 >= udom.first && yval2 <= udom.second && yVals.getBit(yval2) && yval2 != i))
		return true;
	    }
	  }    
	  return false;
	}
	else {
	  if (up) {
	    if (p.at(N) == 0) {
	      // filter x variable
	      MDDBSValue yVals = c.getBS(ySomeUp);
	      MDDBSValue zVals = c.getBS(zSomeUp);
	      for (int i=udom.first; i<=udom.second; i++) {
		const int yofs = i - minDom;
		if (yVals.getBit(yofs) && val != i) {
		  int zval1 = val-i;
		  int zval2 = i-val;
		  if ((zval1 >= udom.first && zval1 <= udom.second && zVals.getBit(zval1)) ||
		      (zval2 >= udom.first && zval2 <= udom.second && zVals.getBit(zval2)))
		    return true;
		}
	      }    
	      return false;
	    }
	    else if (p.at(N) == 1) {
	      // filter y variable
	      MDDBSValue xVals = p.getBS(xSome);
	      MDDBSValue zVals = c.getBS(zSomeUp);	  
	      for (int i=udom.first; i<=udom.second; i++) {
		const int xofs = i - minDom;
		if (xVals.getBit(xofs) && i != val) {
		  int zval1 = val-i;
		  int zval2 = i-val;
		  if ((zval1 >= udom.first && zval1 <= udom.second && zVals.getBit(zval1)) ||
		      (zval2 >= udom.first && zval2 <= udom.second && zVals.getBit(zval2)))
		    return true;
		}
	      }    
	      return false;
	    }
	  }
	}
	return true;
      });
      
      mdd.addRelaxation(xSome,[xSome](auto& out,const auto& l,const auto& r)     {
                                out.getBS(xSome).setBinOR(l.getBS(xSome),r.getBS(xSome));
                            });
      mdd.addRelaxation(ySome,[ySome](auto& out,const auto& l,const auto& r)     {
                                out.getBS(ySome).setBinOR(l.getBS(ySome),r.getBS(ySome));
                            });
      mdd.addRelaxation(ySomeUp,[ySomeUp](auto& out,const auto& l,const auto& r)     {
                                out.getBS(ySomeUp).setBinOR(l.getBS(ySomeUp),r.getBS(ySomeUp));
                            });
      mdd.addRelaxation(zSomeUp,[zSomeUp](auto& out,const auto& l,const auto& r)     {
                                out.getBS(zSomeUp).setBinOR(l.getBS(zSomeUp),r.getBS(zSomeUp));
                            });

      // // This strategy has no impact on performance
      // mdd.splitOnLargest([=](const auto& in) {
      // 	  if (in.getState().at(N) == 1) {
      // 	    MDDBSValue xVals = in.getState().getBS(xSome);
      // 	    return -(double) xVals.cardinality();
      // 	  } else if (in.getState().at(N) == 2) {
      // 	    MDDBSValue yVals = in.getState().getBS(ySome);
      // 	    return -(double) yVals.cardinality();
      // 	  } else
      // 	    return (double)0;
      // 	});
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
     // mdd->saveGraph();
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
       unsigned i = 0u;
       for(i=0u;i < xVars.size();i++)
       	if (xVars[i]->size()> 1) break;
       auto x = i< xVars.size() ? xVars[i] : nullptr;
       
       // // Fail first
       // auto x = selectMin(xVars,
       // 			  [](const auto& x) { return x->size() > 1;},
       //                    [](const auto& x) { return x->size();});

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
       std::cout << "\rNumber of solutions:" << cnt << std::flush;
       // cout << endl;
       // std::cout << "x = " << xVars << endl;
       // std::cout << "y = " << yVars << endl;
     });

      
   // auto stat = search.solve([](const SearchStatistics& stats) {
   //    return stats.numberOfSolutions() > 0;
   // });
   auto stat = search.solve();

   auto end = RuntimeMonitor::cputime();
   cout << stat << endl;
   extern int iterMDD;
   extern int nbCS;
   extern int splitCS,pruneCS,potEXEC;
   extern int nbCONSCall,nbCONSFail;
   extern int nbAECall,nbAEFail;
   extern int hitCS;

   std::cout << "Time : " << RuntimeMonitor::milli(start,end) << '\n';
   std::cout << "I/C  : " << (double)iterMDD/stat.numberOfNodes() << '\n';
   std::cout << "#CS  : " << nbCS << '\n';
   std::cout << "#L   : " << mdd->nbLayers() << '\n';

   extern int splitCS,pruneCS,potEXEC;
   std::cout << "SPLIT:" << splitCS << " \tpruneCS:" << pruneCS << " \tpotEXEC:" << potEXEC << '\n';

   std::cout << "{ \"JSON\" :\n {";
   std::cout << "\n\t\"allInterval\" :" << "{\n";
   std::cout << "\t\t\"size\" : " << N << ",\n";
   std::cout << "\t\t\"m\" : " << mode << ",\n";
   std::cout << "\t\t\"w\" : " << width << ",\n";
   std::cout << "\t\t\"nodes\" : " << stat.numberOfNodes() << ",\n";
   std::cout << "\t\t\"fails\" : " << stat.numberOfFailures() << ",\n";
   std::cout << "\t\t\"iter\" : " << iterMDD << ",\n";
   std::cout << "\t\t\"nbCS\" : " << nbCS << ",\n";
   std::cout << "\t\t\"layers\" : " << mdd->nbLayers() << ",\n";
   std::cout << "\t\t\"splitCS\" : " << splitCS << ",\n";
   std::cout << "\t\t\"pruneCS\" : " << pruneCS << ",\n";
   std::cout << "\t\t\"pot\" : " << potEXEC << ",\n";  
   std::cout << "\t\t\"hitCS\" : " << hitCS << ",\n";  
   std::cout << "\t\t\"time\" : " << RuntimeMonitor::milli(start,end) << "\n";
   std::cout << "\t}\n";  
   std::cout << "}\n}";

   //cp.dealloc();
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
