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
#include <fstream>
#include "solver.hpp"
#include "trailable.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"

int main(int argc,char* argv[])
{
    using namespace std;
    using namespace Factory;

    const int n = 6;
    const int sumResult = n * (n * n + 1) / 2;
    
    CPSolver::Ptr cp  = Factory::makeSolver();
    matrix<var<int>::Ptr,2> x({n,n});
    for(int i=0;i < n;i++)
       for(int j=0;j < n;j++)
          x[i][j] = Factory::makeIntVar(cp,1,n*n);

    cp->post(Factory::allDifferent(x.flat()));    
    for(int i=0;i<n;i++)
       cp->post(sum(slice<var<int>::Ptr>(0,n,[i,&x](int j) { return x[i][j];}),sumResult));
    for(int j=0;j<n;j++)
       cp->post(sum(slice<var<int>::Ptr>(0,n,[j,&x](int i) { return x[i][j];}),sumResult));
    cp->post(sum(slice<var<int>::Ptr>(0,n,[&x](int i) { return x[i][i];}),sumResult));
    cp->post(sum(slice<var<int>::Ptr>(0,n,[n,&x](int i) { return x[n-i-1][i];}),sumResult));

    DFSearch search(cp,firstFail(cp,x.flat()));
    std::vector<int> best(n);

    search.onSolution([&x]() {
                         for(int i=0;i < x.size(0);i++) {
                            for(int j=0;j < x.size(1);j++)
                               cout << std::setw(3) << x[i][j]->min() << " ";
                            cout << std::endl;
                         }
                      });

    auto stat = search.solve([](const SearchStatistics& stats) {
                                return stats.numberOfSolutions() >= 1;
                             });
    cout << stat << endl;    
    cp.dealloc();
    return 0;
}
