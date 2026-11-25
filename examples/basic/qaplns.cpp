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
#include <random>
#include "core/solver.hpp"
#include "core/trailable.hpp"
#include "core/intvar.hpp"
#include "core/constraint.hpp"
#include "core/search.hpp"

int main(int argc,char* argv[])
{
    using namespace std;
    using namespace Factory;

    ifstream data("data/qap/qap.txt");
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

    CPSolver::Ptr cp  = Factory::makeSolver();
    auto x = Factory::intVarArray(cp,n,n);
    cp->post(Factory::allDifferent(x));
    auto wDist = Factory::intVarArray(cp,n*n);
    for(int k=0,i=0;i < n;i++)
       for(int j=0;j < n;j++)
          wDist[k++] = w[i][j] * Factory::element(d,x[i],x[j]);
    Objective::Ptr obj = Factory::minimize(Factory::sum(wDist));

    DFSearch search(cp,firstFail(cp,x));
    std::vector<int> best(n);
    bool hasBest = false;   // best holds a solution only once one was found
    int bestValue = 0;      // its objective value (obj->value() is not meaningful after the search)

    search.onSolution([&x,&obj,&best,&hasBest,&bestValue,n]() {
                         for(int i=0;i<n;i++)
                            best[i] = x[i]->min();
                         hasBest = true;
                         bestValue = obj->value();
                         cout << "objective = " << bestValue << endl;
                      });

    int nRestarts = 1000;
    int failureLimit = 100;
    std::random_device rand;  
    std::mt19937 gen(rand()); 
    std::uniform_int_distribution<> dist(0, 99);   // dist(gen) < 25: exactly 25%

    for(int i=0;i<nRestarts;i++) {
       if (i%10==0)
          std::cout << "restart number #" << i << std::endl;
       search.optimizeSubjectTo(obj,
                                [&failureLimit](const SearchStatistics& stats) {
                                   return stats.numberOfFailures() >= failureLimit;
                                },
                                [n,cp,&x,&best,&hasBest,&dist,&gen] {
                                   if (!hasBest) return;   // no incumbent yet: search from scratch
                                   for(int j=0;j < n;j++)
                                      if (dist(gen) < 25)
                                         cp->post(x[j] == best[j]);
                                });
    }
    if (hasBest) {
       cout << "Best: " << bestValue << endl;
       cout << "Assignment:";
       for(int i=0;i<n;i++)
          cout << ' ' << best[i];
       cout << endl;
    } else
       cout << "Best: none found" << endl;
    cp.dealloc();
    return 0;
}
