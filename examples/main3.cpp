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
            cp->post(Factory::makeNEQBinBC(q[i],q[j],i-j));            
            cp->post(Factory::makeNEQBinBC(q[i],q[j],j-i));            
        }
    cp->optimize(Factory::minimize(q[n-1]));

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
