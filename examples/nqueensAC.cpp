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

#include "alldifferent.hpp"
#include "search.hpp"
#include "utils.hpp"
#include "gfl/Alias.hpp"
#include <CLI/CLI.hpp>

#include <iostream>

using namespace std;
using namespace Factory;
using namespace gfl;

// The n-queens problem: place n queens on an n x n board so that no two share a
// row, a column or a diagonal. One variable per column holds the row of its
// queen, which takes care of the columns; the three alldifferents rule out a
// shared row and a shared diagonal in either direction.
//
// Counting every solution keeps the propagators busy over a large search tree,
// which is what makes this a reasonable workout for the alldifferent the -g
// flag moves to the GPU. The board has to be big for that to pay off: the
// residual graph only gets dense enough to beat Tarjan's algorithm once there
// are many queens sharing many rows.

int main(int argc, char * argv[])
{
    // Parse the command line
    auto n = 8;
    auto timeout = highest<int>(); // ~68 years
    auto firstOnly = false;
    auto gpu = false;

    CLI::App app{"A MiniCPP-based n-queens solver"};
    app.add_option("-n", n, "How many queens")->check(CLI::PositiveNumber);
    app.add_option("-t", timeout, "Timeout in seconds");
    app.add_flag("-1", firstOnly, "Stop at the first solution instead of counting them all");
    app.add_flag("-g", gpu, "Run the alldifferent constraints on the GPU");
    CLI11_PARSE(app, argc, argv);

    auto const dev = gpu ? GPU : CPU;

    CPSolver::Ptr cp  = Factory::makeSolver();
    auto q = Factory::intVarArray(cp,n,1,n);
    cp->post(Factory::allDifferentAC(q,dev));
    cp->post(Factory::allDifferentAC(Factory::intVarArray(cp,n,[q](int i) {
                                                                  return q[i] + i;
                                                               }),dev));
    cp->post(Factory::allDifferentAC(Factory::intVarArray(cp,n,[q](int i) {
                                                                  return q[i] - i;
                                                               }),dev));

    DFSearch search(cp,[=]() {
                          auto x = selectMin(q,
                                             [](const auto& x) { return x->size() > 1;},
                                             [](const auto& x) { return x->size();});
                          if (x) {
                             int c = x->min();                    
                             return  [=] { cp->post(x == c);}
                                | [=] { cp->post(x != c);};
                          } else return Branches({});
                       });    

    //search.onSolution([&q]() { cout << "sol = " << q << endl;});

    auto limits = [=](SearchStatistics const & ss) {
        return RuntimeMonitor::elapsedSeconds(ss.getStartTime()) > timeout || (firstOnly && ss.getSolutions() > 0);
    };
    auto stat = search.solve(limits);
    cout << stat << endl;
    
    cp.dealloc();
    return 0;
}
