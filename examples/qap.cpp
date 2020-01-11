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

    ifstream data("data/qap.txt");
    int n;
    data >> n;
    Matrix<int,2> w({n,n});
    for(int i =0;i < n;i++)
       for(int j=0;j < n;j++)
          data >> w[i][j];
    Matrix<int,2> d({n,n});
    for(int i =0;i < n;i++)
       for(int j=0;j < n;j++)
          data >> d[i][j];

    /*
    for(int i =0;i < n;i++) {
       for(int j=0;j < n;j++)
          cout << setw(3) << w[i][j] << " ";
       cout << endl;
    }
    for(int i =0;i < n;i++) {
       for(int j=0;j < n;j++)
          cout << setw(3) << d[i][j] << " ";
       cout << endl;
    }
    */

    
    
    CPSolver::Ptr cp  = Factory::makeSolver();
    auto x = Factory::intVarArray(cp,n,n);
    cp->post(Factory::allDifferent(x));
    auto wDist = Factory::intVarArray(cp,n*n);
    for(int k=0,i=0;i < n;i++)
       for(int j=0;j < n;j++)
          wDist[k++] = w[i][j] * Factory::element(d,x[i],x[j]);
    Objective::Ptr obj = Factory::minimize(Factory::sum(wDist));
       
    DFSearch search(cp,firstFail(cp,x));

    search.onSolution([&x,&obj]() {
                         cout << "objective = " << obj->value() << endl;
                      });

    auto stat = search.optimize(obj);
    cout << stat << endl;    
    cp.dealloc();
    return 0;
}
