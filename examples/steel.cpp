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
#include <list>
#include <set>
#include <numeric>
#include "solver.hpp"
#include "trailable.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"

int main(int argc,char* argv[])
{
    using namespace std;
    using namespace Factory;

    ifstream data("data/steel/bench_19_10");
    int nCapa;
    data >> nCapa;
    std::vector<int> capa(nCapa);
    for(int i=0;i<nCapa;i++) 
       data >> capa[i];
    int maxCapa = capa[capa.size() - 1];
    std::vector<int> loss(maxCapa + 1);
    int capaIdx = 0;
    for(int i=0;i < maxCapa;i++) {
       loss[i] = capa[capaIdx] - i;
       if (loss[i] == 0) capaIdx++;
    }
    loss[0] = 0;
    int nCol,nSlab;
    data >> nCol >> nSlab;
    int nOrder = nSlab;
    std::vector<int> w(nSlab);
    std::vector<int> c(nSlab);
    for(int i=0;i < nSlab;i++) {
       data >> w[i] >> c[i];
       c[i] -= 1;
    }

    CPSolver::Ptr cp  = Factory::makeSolver();
    auto x = Factory::intVarArray(cp,nOrder,nSlab);
    auto l = Factory::intVarArray(cp,nSlab,maxCapa + 1);

    Matrix<var<bool>::Ptr,2> inSlab({nSlab,nOrder});
    for(int j=0;j < nSlab;j++)
       for(int i=0;i < nOrder;i++)
          inSlab[j][i] = isEqual(x[i],j);

    for(int j=0;j < nSlab;j++) {
       auto presence = boolVarArray(cp,nCol);
       for(int col=0; col < nCol;col++) {
          presence[col] = makeBoolVar(cp);
          std::vector<var<bool>::Ptr> inSlabWithColor;
          for(int i=0;i < nOrder;i++)
             if (c[i] == col)
                inSlabWithColor.push_back(inSlab[j][i]);
          cp->post(isClause(presence[col],inSlabWithColor));
       }
       cp->post(sum(presence) <= 2);
    }
    for(int j=0;j < nSlab;j++) {
        auto wj = Factory::intVarArray(cp,nSlab);
        for(int i=0;i < nOrder;i++)
            wj[i] = inSlab[j][i] * w[i];
        cp->post(sum(wj,l[j]));
    }
    cp->post(sum(l,std::accumulate(w.begin(),w.end(),0)));
    auto losses = Factory::intVarArray(cp,nSlab,[loss,l](int j) { return Factory::element(loss,l[j]);});
    auto totLoss = Factory::sum(losses);
    Objective::Ptr obj = Factory::minimize(totLoss);
   
    DFSearch search(cp,[cp,x]() {
                           auto xs = selectMin(x,
                                               [](const auto& x) { return x->size() > 1;},
                                               [](const auto& x) { return x->size();});
                           if (xs) {
                               int maxUsed = -1;
                               for(auto xi : x)
                                   if (xi->isBound() && xi->min() > maxUsed)
                                       maxUsed = xi->min();
                               std::vector<function<void(void)>> br;
                               for(int i=0;i <= maxUsed + 1;i++)
                                   br.push_back([cp,xs,i] { return cp->post(xs == i);});
                               return Branches(br);
                           } else return Branches({});                             
                       });
   
    search.onSolution([&x,&c,&totLoss,nSlab,nOrder]() {
                         cout << "---";
                         std::vector<std::set<int>>  colorsInSlab;                         
                         for(int j=0;j < nSlab;j++)
                             colorsInSlab.push_back(std::set<int>());
                         for(int i=0;i < nOrder;i++)
                             colorsInSlab[x[i]->min()].insert(c[i]);
                         for(int j=0;j < nSlab;j++) {
                             if (colorsInSlab[j].size() > 2)
                                 cout << "THERE IS A BUG!" << endl;
                         }
                         cout << "Total loss = " << totLoss->min() << endl;
                      });
    
    auto stat = search.optimize(obj);
    cout << stat << endl;    
    cp.dealloc();
    return 0;
}
