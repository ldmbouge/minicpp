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

    ifstream data("data/stable_marriage.txt");
    int n;
    data >> n;
    matrix<int,2> rankWomen({n,n}); 
    matrix<int,2> rankMen({n,n});
    for(int i =0;i < n;i++)
       for(int j=0;j < n;j++)
          data >> rankWomen[i][j];
    for(int i =0;i < n;i++)
       for(int j=0;j < n;j++)
          data >> rankMen[i][j];

    CPSolver::Ptr cp  = Factory::makeSolver();
    auto wife    = Factory::intVarArray(cp,n,n);
    auto husband = Factory::intVarArray(cp,n,n);
    auto wifePref = Factory::intVarArray(cp,n,n+1); 
    auto husbandPref = Factory::intVarArray(cp,n,n+1);

    for(int m=0;m < n;m++) {
       //cp->post(Factory::elementVar(husband,wife[m]) == m);
       cp->post(husband[wife[m]] == m);
       cp->post(Factory::element(rankWomen[m],wife[m],wifePref[m]));
    }
    for(int w=0;w < n;w++) {
       //cp->post(Factory::elementVar(wife,husband[w]) == w);
       cp->post(wife[husband[w]] == w);
       cp->post(Factory::element(rankMen[w],husband[w],husbandPref[w]));
    }
    for(int m=0; m < n;m++) {
       for(int w=0;w < n;w++) {
          auto mPrefersW = isLarger(wifePref[m],rankWomen[m][w]);
          auto wDont = isLess(husbandPref[w],rankMen[w][m]);
          cp->post(implies(mPrefersW,wDont) == true);

          auto wPrefersM = isLarger(husbandPref[w],rankMen[w][m]);
          auto mDont = isLess(wifePref[m],rankWomen[m][w]);
          cp->post(implies(wPrefersM,mDont) == true);
       }
    }
    
    DFSearch search(cp,land({firstFail(cp,wife),firstFail(cp,husband)}));
   
    search.onSolution([&wife,&husband]() {
                         cout << "wife = ";
                         for(int i=0;i < wife.size();i++)
                            cout << setw(4) << wife[i] << " ";
                         cout << " | husband = ";
                         for(int i=0;i < husband.size();i++)
                            cout << setw(4) << husband[i] << " ";
                         cout << endl;
                      });
    
    auto stat = search.solve();
    cout << stat << endl;    
    cp.dealloc();
    return 0;
}
