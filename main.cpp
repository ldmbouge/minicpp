#include <iostream>
#include <iomanip>
#include "solver.hpp"
#include "reversible.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"

int main(int argc,char* argv[])
{
    using namespace std;
    using namespace Factory;
    CPSolver::Ptr cp  = Factory::makeSolver();
    const int n = argc >= 2 ? atoi(argv[1]) : 12;
    const bool one = argc >= 3 ? atoi(argv[2])==0 : false;
    auto q = Factory::intVarArray(cp,n,1,n);
    for(int i=0;i < n;i++)
        for(int j=i+1;j < n;j++) {
            cp->add(q[i] != q[j]);            
            cp->add(Factory::makeNEQBinBC(q[i],q[j],i-j));            
            cp->add(Factory::makeNEQBinBC(q[i],q[j],j-i));            
        }
    
    cp->close();
    Chooser c([=] {
            return selectMin(q,
                             [](const var<int>::Ptr& x) { return x->getSize() > 1;},
                             [](const var<int>::Ptr& x) { return x->getSize();},
                             [cp](const var<int>::Ptr& x) {
                                 int c = x->getMin();                    
                                 return  [=] { cp->add(x == c);}
                                       | [=] { cp->add(x != c);};
                             });
        });

    int nbSol = 0;
    if (one) {
       try {
          dfsAll(cp,c,[&] {
                cout << "sol = " << q << endl;
                nbSol++;
                throw 0;
             });
       } catch(int x) {
          cout << "stopped..." << endl;
       }
    } else {
       dfsAll(cp,c,[&] {
             //cout << "sol = " << q << endl;
             nbSol++;
          });
    }
    
    cout << "Got: " << nbSol << " solutions" << endl;
    cout << *cp << endl;
    cp.dealloc();
    return 0;
}
