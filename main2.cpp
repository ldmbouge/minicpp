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

    shared_ptr<int>  nbSol = make_shared<int>(0);
    if (one) { 
       cp->solveOne([&] {
             for(int i=0;i < n;i++) {
                auto  cx = min_dom(q);
                if (cx == q.end()) break;
                auto& x = *cx;
                while(!x->isBound()) {
                   int c = x->getMin();
                   cp->tryBin([&] { cp->add(x == c);},
                              [&] { cp->add(x != c);});
                }
             }
             cout << q << endl;
             cp->incrNbSol();
             *nbSol += 1;
          });
    } else {
       cp->solveAll([&] {
             for(int i=0;i < n;i++) {
                withVarDo(q,min_dom(q),[&](auto& x) {
                      while(!x->isBound()) {
                         int c = x->getMin();
                         cp->tryBin([&] { cp->add(x == c);},
                                    [&] { cp->add(x != c);});
                      }                      
                   });
             }
             cp->incrNbSol();
             *nbSol += 1;
          });
    }
    
    cout << "Got: " << *nbSol << " solutions" << endl;
    cout << *cp << endl;
    return 0;
}
