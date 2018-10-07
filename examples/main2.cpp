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
   auto q = Factory::intVarArray(cp,n,1,n);
   for(int i=0;i < n;i++)
      for(int j=i+1;j < n;j++) {
         cp->post(q[i] != q[j]);            
         cp->post(Factory::notEqual(q[i],q[j],i-j));            
         cp->post(Factory::notEqual(q[i],q[j],j-i));            
      }

   DFSearch search(cp,[=]() {
                         auto x = selectMin(q,
                                            [](const auto& x) { return x->size() > 1;},
                                            [](const auto& x) { return x->size();});
                         if (x) {
                            int c = x->min();                    
                            return  [=] { cp->post(x == c);}
                                  | [=] { cp->post(x != c);};
                         } else return Branches({});
                      });
   int nbSol = 0;
   search.onSolution([&nbSol]() { nbSol++;});
   auto stat = search.solve();
   
   cout << "Got: " << nbSol << " solutions" << endl;
   cout << stat << endl;
   cp.dealloc();
   return 0;
}
