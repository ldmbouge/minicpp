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
#include "solver.hpp"
#include "trailable.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"

void pdebug(std::vector<var<int>::Ptr>& x)
{
   std::cout << "DEBUG:" << x << std::endl;
}

template<class ForwardIt> ForwardIt min_dom(ForwardIt first, ForwardIt last)
{
   if (first == last) return last;

   int ds = 0x7fffffff;
   ForwardIt smallest = last;
   for (; first != last; ++first) {
      auto fsz = (*first)->size();
      if (fsz > 1 && fsz < ds) {
         smallest = first;
         ds = fsz;
      }
   }
   return smallest;
}

template<class Container> auto min_dom(Container& c) {
   return min_dom(c.begin(),c.end());
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
         cp->post(q[i] != q[j]);            
         cp->post(Factory::notEqual(q[i],q[j],i-j));            
         cp->post(Factory::notEqual(q[i],q[j],j-i));            
      }
       
   auto solve = one ? &CPSolver::solveOne : &CPSolver::solveAll;
   int* nbSol = new (cp) int(0);    // allocate the integer on the solver allocator
   //cp->solveOne([&] {
   //cp->solveAll([&] {
   (*cp.*solve)([&] {
         for(int i=0;i < n;i++) {
	    withVarDo(q,min_dom(q),[cp](auto x) {
                  while(!x->isBound()) {
                     int c = x->min();
                     cp->tryBin([cp,x,c] { cp->branch(Factory::operator==(x,c));},
  			        [cp,x,c] { cp->branch(Factory::operator!=(x,c));});
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
