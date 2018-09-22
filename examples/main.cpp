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

int main(int argc,char* argv[])
{
    using namespace std;
    using namespace Factory;
    CPSolver::Ptr cp  = Factory::makeSolver();
    const int n = argc >= 2 ? atoi(argv[1]) : 12;
    const bool one = argc >= 3 ? atoi(argv[2])==0 : false;
    auto q = Factory::intVarArray(cp,n,1,n);
    for(int i=0;i < n;i++)
        for(int j=i+1;j < n;j++) {
            cp->post(q[i] != q[j]);            
            cp->post(Factory::notEqual(q[i],q[j],i-j));            
            cp->post(Factory::notEqual(q[i],q[j],j-i));            
        }
    Objective::Ptr obj = Factory::minimize(q[n-1]);
    
    Chooser c([=] {
                  auto x =  selectMin(q,
                                      [](const auto& x) { return x->size() > 1;},
                                      [](const auto& x) { return x->size();});
                  if (x) {
                      int c = x->min();                    
                      return  [=] { cp->post(x == c);}
                            | [=] { cp->post(x != c);};
                  } else return Branches({});
              });

    int nbSol = 0;
    if (one) {
       try {
          dfsAll(cp,c,[&] {
                cout << "sol = " << q << endl;
                nbSol++;
                obj->tighten();
                throw 0;
             });
       } catch(int x) {
          cout << "stopped..." << endl;
       }
    } else {
       dfsAll(cp,c,[&] {
             cout << "sol = " << q << endl;
             nbSol++;
             obj->tighten();
          });
    }
    
    cout << "Got: " << nbSol << " solutions" << endl;
    cout << *cp << endl;
    cp.dealloc();
    return 0;
}
