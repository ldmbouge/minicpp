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

// Quadratic assignment by LNS, written with the lns combinator of search.hpp
// instead of qaplns.cpp's hand-written loop of optimizeSubjectTo calls:
//  - capture records each solution;
//  - freeze fixes a random fragment (about `percent`%) of the last solution;
//    lns skips it until a first solution exists, and a fragment that fails
//    at the root simply starts the next iteration;
//  - each iteration runs firstFail under a failure limit;
//  - the search-wide Limit stops after nRestarts neighbourhoods.
//
// Usage:
//   qaplns2 [nRestarts (1000)] [failureLimit (100)] [percent (25)] [seed (random)]

#include <iostream>
#include <iomanip>
#include <fstream>
#include <random>
#include <cstdlib>
#include "core/solver.hpp"
#include "core/trailable.hpp"
#include "core/intvar.hpp"
#include "core/constraint.hpp"
#include "core/search.hpp"

int main(int argc,char* argv[])
{
    using namespace std;
    using namespace Factory;

    int const nRestarts    = argc > 1 ? atoi(argv[1]) : 1000;
    int const failureLimit = argc > 2 ? atoi(argv[2]) : 100;
    int const percent      = argc > 3 ? atoi(argv[3]) : 25;
    unsigned const seed    = argc > 4 ? (unsigned)atoi(argv[4]) : std::random_device{}();

    ifstream data("data/qap/qap.txt");
    if (!data) {
       cerr << "cannot open data/qap/qap.txt" << endl;
       return 2;
    }
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

    // capture: the solution DFS is about to report (every x is bound at a leaf).
    std::vector<int> best(n);
    auto capture = [&] {
       for(int i=0;i<n;i++)
          best[i] = x[i]->min();
    };

    // freeze: fix each x[j] to its value in the last solution with probability percent%.
    std::mt19937 gen(seed);
    std::uniform_int_distribution<> dist(0, 99);
    int nbRestarts = 0;
    auto freeze = [&] {
       if (nbRestarts % 10 == 0)
          cout << "restart number #" << nbRestarts << endl;
       nbRestarts++;
       for(int j=0;j < n;j++)
          if (dist(gen) < percent)
             cp->post(x[j] == best[j]);
    };

    DFSearch search(cp,lns(cp,capture,freeze,firstFail(cp,x),
                           [failureLimit](const SearchStatistics& s) {
                              return s.numberOfFailures() >= failureLimit;
                           }));

    // The best value is recorded here: once optimize returns, the search is back
    // at the root and obj->value() no longer reflects the incumbent.
    int bestValue = 0;
    bool found = false;
    search.onSolution([&obj,&nbRestarts,&bestValue,&found]() {
       bestValue = obj->value();
       found = true;
       cout << "objective = " << bestValue << "\t(restart " << nbRestarts << ")" << endl;
    });

    // lns runs until stopped: stop once nRestarts neighbourhoods have been started.
    auto stat = search.optimize(obj,[&nbRestarts,nRestarts](const SearchStatistics&) {
       return nbRestarts >= nRestarts;
    });

    cout << "Restarts: " << nbRestarts << endl;
    if (found) {
       // best is the last captured solution, i.e. the incumbent (each one improves).
       cout << "Best: " << bestValue << endl;
       cout << "Assignment:";
       for(int i=0;i<n;i++)
          cout << ' ' << best[i];
       cout << endl;
    } else
       cout << "Best: none found" << endl;
    cout << stat << endl;
    cp.dealloc();
    return 0;
}
