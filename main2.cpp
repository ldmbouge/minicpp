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
    auto solve = one ? &CPSolver::solveOne : &CPSolver::solveAll;
    shared_ptr<int>  nbSol = make_shared<int>(0);
    //cp->solveOne([&] {
    //cp->solveAll([&] {
    (*cp.*solve)([cp,q,n,nbSol] {
          for(int i=0;i < n;i++) {
             withVarDo(q,min_dom(q),[cp](auto x) {
                   while(!x->isBound()) {
                      int c = x->getMin();
                      cp->tryBin([=] { cp->add(x == c);},
                                 [=] { cp->add(x != c);});
                   }                      
                });
          }
          cp->incrNbSol();
          //cout << q << endl;
          *nbSol += 1;
       });
    
    cout << "Got: " << *nbSol << " solutions" << endl;
    cout << *cp << endl;
    cp.dealloc();
    return 0;
}
