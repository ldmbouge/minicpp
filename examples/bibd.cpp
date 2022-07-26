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
#include "range.hpp"
#include "lex.hpp"
#include <string>

void showMatrix(Matrix<var<bool>::Ptr,2>& M)
{
   std::cout << std::string(72,'-') << '\n';
   for(int i=0;i < M.size(0);i++) {
      for(int j=0;j < M.size(1);j++)
         if (M[i][j]->isBound())
            std::cout << std::setw(2) << M[i][j]->min() << " ";
         else
            std::cout << std::setw(2) << '?' << " ";
      std::cout << '\n';
   }
}

int main(int argc,char* argv[])
{
   using namespace Factory;
   int a = (argc >= 2 && strncmp(argv[1],"-n",2)==0) ? atoi(argv[1]+2) : 1;
   int instances[14][3] = {
      {7,3,1},{6,3,2},{8,4,3},{7,3,20},{7,3,30},
      {7,3,40},{7,3,45},{7,3,50},{7,3,55},{7,3,60},
      {7,3,300},{8,4,5},{8,4,6},{8,4,7}
   };
   const int v = instances[a][0],k=instances[a][1],l=instances[a][2];
   const int b = (v*(v-1)*l)/(k*(k-1));
   const int r = l * (v-1) / (k-1);
   CPSolver::Ptr cp  = Factory::makeSolver();
   auto rows = range(0,v-1),cols = range(0,b-1);

   Matrix<var<bool>::Ptr,2> M({v,b});
   for(int i=0;i < v;i++)
      for(int j=0;j < b;j++)
         M[i][j] = Factory::makeBoolVar(cp);

   for(auto i : rows)
      cp->post(sum(slice<var<bool>::Ptr>(0,b,[&M,i](int j) { return M[i][j];}),r));
   for(auto j : cols)
      cp->post(sum(slice<var<bool>::Ptr>(0,v,[&M,j](int i) { return M[i][j];}),k));
   for(auto i : rows) 
      for(int j=i+1;j < v;j++)
         cp->post(sum(slice<var<bool>::Ptr>(0,b,[&M,i,j](int x) { return M[i][x] * M[j][x];}),l));
   for(int i=0;i < v - 1;i++) {
      cp->post(lexLeq(slice<var<bool>::Ptr>(0,b,[&M,i](int j) { return M[i+1][j];}),
                      slice<var<bool>::Ptr>(0,b,[&M,i](int j) { return M[i  ][j];})));
   }
   
   for(int j=0;j < b - 1;j++) {
      cp->post(lexLeq(slice<var<bool>::Ptr>(0,v,[&M,j](int i) { return M[i][j+1];}),
                      slice<var<bool>::Ptr>(0,v,[&M,j](int i) { return M[i][j];})));
   }

   auto q = M.flat();
   DFSearch search(cp,firstFail(cp,M.flat()));
    
   search.onSolution([&M]() {
      for(int i=0;i < M.size(0);i++) {
         for(int j=0;j < M.size(1);j++)
            std::cout << std::setw(1) << M[i][j]->min() << " ";
         std::cout << '\n';
      }
   });
   
   auto stat = search.solve([](const SearchStatistics& stats) {
       return stats.numberOfSolutions() >= 1;
   });
   std::cout << stat << '\n';
   cp.dealloc();
   return 0;
}
