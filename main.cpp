#include <iostream>
#include <iomanip>
#include "solver.hpp"
#include "reversible.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"

int main()
{
    using namespace std;
    using namespace Factory;
    CPSolver::Ptr cp  = Factory::makeSolver();
    int n = 8;
    auto q = Factory::intVarArray(cp,n,0,n-1);
    for(int i=0;i < n;i++)
        for(int j=i+1;j < n;j++) {
            cp->add(Factory::makeNEQBinBC(q[i],q[j],0));            
            cp->add(Factory::makeNEQBinBC(q[i],q[j],i-j));            
            cp->add(Factory::makeNEQBinBC(q[i],q[j],j-i));            
        }
    
    cp->close();
    Chooser c([=] {
            return selectMin(q,
                             [](var<int>::Ptr x) { return x->getSize() > 1;},
                             [](var<int>::Ptr x) { return x->getSize();},
                             [cp](var<int>::Ptr x) {
                                 int c = x->getMin();                    
                                 return  [=] { cp->add(x == c);}
                                       | [=] { cp->add(x != c);};
                             });
        });

    int nbSol = 0;
    dfsAll(cp,c,[&] {
            cout << "sol = " << q << endl;
            nbSol++;
        });
    
    cout << "Got: " << nbSol << " solutions" << endl;
    cout << *cp << endl;
    return 0;
}
