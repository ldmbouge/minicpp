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
    const int n = 8;
    CPSolver::Ptr cp  = Factory::makeSolver();
    auto q = Factory::intVarArray(cp,n,1,n);
    for(int i=0;i < n;i++)
        for(int j=i+1;j < n;j++) {
            cp->post(q[i] != q[j]);            
            cp->post(Factory::notEqual(q[i],q[j],i-j));            
            cp->post(Factory::notEqual(q[i],q[j],j-i));            
        }

    auto q1 = Factory::intVarArray(cp,n/2);
    auto q2 = Factory::intVarArray(cp,n/2);

    for(int i=0;i < n/2;i++) {
        q1[i] = q[i];
        q2[i] = q[n/2+i];
    }
    
    DFSearch search(cp,land({firstFail(cp,q1),firstFail(cp,q2)}));
    //DFSearch search(cp,firstFail(cp,q));
    /*    DFSearch search(cp,[=]() {
                           auto x = selectMin(q,
                                              [](const auto& x) { return x->size() > 1;},
                                              [](const auto& x) { return x->size();});
                           if (x) {
                               int c = x->min();                    
                               return  [=] { cp->post(x == c);}
                                     | [=] { cp->post(x != c);};
                           } else return Branches({});
                       });
    */
    int nbSol = 0;
    search.onSolution([&nbSol,&q]() {
                         cout << "sol = " << q << endl;
                         nbSol++;
                      });

    auto stat = search.solve();
    cout << stat << endl;
    
    cout << "Got: " << nbSol << " solutions" << endl;
    
    cp.dealloc();
    return 0;
}
