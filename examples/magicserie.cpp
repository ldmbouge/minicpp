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
    const int n = 300;
    CPSolver::Ptr cp  = Factory::makeSolver();
    auto s = Factory::intVarArray(cp,n,n);
    for(int i=0;i < n;i++) {
        cp->post(sum(Factory::boolVarArray(cp,n,[&s,i](int j) { return Factory::isEqual(s[j],i);}),s[i]));
    }
    cp->post(sum(s,n));
    //cp->post(sum(Factory::intVarArray(cp,n,[&s](int i) { return s[i]*i;}),n));
    
    DFSearch search(cp,[=]() {
                          auto x = selectMin(s,
                                             [](const auto& x) { return x->size() > 1;},
                                             [](const auto& x) { return x->size();});
                          if (x) {
                             int c = x->min();                    
                             return  [=] { cp->post(x == c);}
                                | [=] { cp->post(x != c);};
                          } else return Branches({});
                       });    

    search.onSolution([&s]() {
                         cout << "sol = " << s << endl;
                      });

    auto stat = search.solve();
    cout << stat << endl;    
    cp.dealloc();
    return 0;
}
