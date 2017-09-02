#include <iostream>
#include <iomanip>
#include "solver.hpp"
#include "reversible.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"

void pdebug(std::vector<var<int>::Ptr>& x)
{
   std::cout << "DEBUG:" << x << std::endl;
}

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
   int* nbSol = new (cp) int(0);    // allocate the integer on the solver allocator
   //cp->solveOne([&] {
   //cp->solveAll([&] {
   (*cp.*solve)([&] {
         for(int i=0;i < n;i++) {
	    withVarDo(q,min_dom(q),[cp](auto x) {
                  while(!x->isBound()) {
                     int c = x->getMin();
                     cp->tryBin([cp,x,c] { cp->add(x == c);},
                                [cp,x,c] { cp->add(x != c);});
                  }                      		
               });
         }
         cp->incrNbSol();
         //cout << q << endl;
         *nbSol += 1;
      });
   //cout << q << endl;
      
   cout << "Got: " << *nbSol << " solutions" << endl;
   cout << *cp << endl;
   cp.dealloc();
   return 0;
}
