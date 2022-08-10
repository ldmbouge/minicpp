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
 * Copyright (c)  2018. by Laurent Michel, Willem Jan van Hoeve, Pierre Schaus, Pascal Van Hentenryck
 */

#include <iostream>
#include <iomanip>
#include <string.h>
#include <limits>
#include "solver.hpp"
#include "trailable.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"
#include "mdd.hpp"
#include "mddrelax.hpp"
#include "mddConstraints.hpp"
#include "RuntimeMonitor.hpp"
#include "table.hpp"
#include "range.hpp"

// ------------------------------------------------------------
// Utility functions

/**
 * Maps the values in the input set (in the given order) to entries of an array.
 * @param cp the solver to allocate the resulting array
 * @param over the ordered set of integer values to map
 * @param clo is the first-order function to carry out the mapping
 * @return an array [clo(i0),clo(i1),...,clo(in)] for the ordered set {i0,i1,...,in}
 */
Factory::Veci map(CPSolver::Ptr cp,const std::set<int>& over, std::function<var<int>::Ptr(int)> clo)
{
   auto res = Factory::intVarArray(cp, (int) over.size());
   int i = 0;
   for(auto e : over)
      res[i++] = clo(e);
   return res;
}

/**
 * Filter the values in the container c, placing into the result set only those that satisfy the predicate
 * @param c a container of values fo filter
 * @param pred the unary Boolean predicate that must be satisfied in order for a value in c to be included
 * @return the subset of values that satisfy the predicate.
 */
template <class Container,typename UP>
std::set<typename Container::value_type> filter(const Container& c,const UP& pred)
{
   std::set<typename Container::value_type> r;
   for(auto e : c)
      if (pred(e))
         r.insert(e);
   return r;
}

int main(int argc,char* argv[])
{
   using namespace Factory;
   int N         = (argc >= 2 && strncmp(argv[1],"-n",2)==0) ? atoi(argv[1]+2) : 8;
   int width     = (argc >= 3 && strncmp(argv[2],"-w",2)==0) ? atoi(argv[2]+2) : 1;
   int maxReboot = (argc >= 4 && strncmp(argv[3],"-r",2)==0) ? atoi(argv[3]+2) : 0;

   std::cout << "N = " << N << "\n";   
   std::cout << "width = " << width << "\n";   
   std::cout << "max reboot distance = " << maxReboot << "\n";

   CPSolver::Ptr cp  = Factory::makeSolver();

   auto v = Factory::intVarArray(cp, 2*N-1, 0, N-1);

   std::set<int> xIdx = filter(range(0,2*N-2),[](int i) {return i==0 || i%2!=0;});
   std::set<int> yIdx = filter(range(2,2*N-2),[](int i) {return i%2==0;});
   auto x = map(cp, xIdx, [&v](int i) {return v[i];});
   auto y = map(cp, yIdx, [&v](int i) {return v[i];});

   for (auto i=0u; i<y.size(); i++) 
      cp->post(y[i] != 0);
   auto mdd = Factory::makeMDDRelax(cp,width,maxReboot,10,0,0);
   mdd->post(Factory::allDiffMDD(x));
   mdd->post(Factory::allDiffMDD(y));
   for(int i=0; i < N-1;i++)
      mdd->post(Factory::absDiffMDD(mdd,{y[i],x[i+1],x[i]}));
   cp->post(mdd);

   DFSearch search(cp,[=]() { 
      return indomain_min(cp,selectFirst(x,[](const auto& x) { return x->size() > 1;}));
   });
   SearchStatistics stat = search.solve();
   std::cout << stat << "\n";
   return 0;
}
