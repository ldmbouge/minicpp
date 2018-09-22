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

    ifstream data("data/tsp.txt");
    int n;
    data >> n;
    matrix<int,2> distanceMatrix({n,n});
    for(int i =0;i < n;i++)
       for(int j=0;j < n;j++)
          data >> distanceMatrix[i][j];

    for(int i =0;i < n;i++) {
       for(int j=0;j < n;j++)
          cout << setw(4) << distanceMatrix[i][j] << " ";
       cout << endl;
    }

    CPSolver::Ptr cp  = Factory::makeSolver();
    auto succ = Factory::intVarArray(cp,n,n);
    auto distSucc = Factory::intVarArray(cp,n,1000);
    cp->post(Factory::circuit(succ));    
    for(int i=0;i < n;i++)
        cp->post(Factory::element(distanceMatrix[i],succ[i],distSucc[i]));

    Objective::Ptr obj = Factory::minimize(Factory::sum(distSucc));
       
    DFSearch search(cp,firstFail(cp,succ));
    std::vector<int> best(n);

    search.onSolution([&succ,&best,&obj]() {
                          for(int i=0;i < best.size();i++)
                              best[i] = succ[i]->min();
                          cout << "objective = " << obj->value() << endl;
                      });

    auto stat = search.optimize(obj);
    cout << stat << endl;    
    cp.dealloc();
    return 0;
}
