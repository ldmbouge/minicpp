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
#include "mdd.hpp"
#include "mddrelax.hpp"
#include "mddConstraints.hpp"
#include "RuntimeMonitor.hpp"
#include "table.hpp"
#include "range.hpp"
#include "allInterval.hpp"

using namespace std;
using namespace Factory;

/***/

Veci all(CPSolver::Ptr cp,const set<int>& over, std::function<var<int>::Ptr(int)> clo)
{
   auto res = Factory::intVarArray(cp, (int) over.size());
   int i = 0;
   for(auto e : over){
      res[i++] = clo(e);
   }
   return res;
}

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

   int N     = (argc >= 2 && strncmp(argv[1],"-n",2)==0) ? atoi(argv[1]+2) : 8;
   int width = (argc >= 3 && strncmp(argv[2],"-w",2)==0) ? atoi(argv[2]+2) : 1;
   int mode  = (argc >= 4 && strncmp(argv[3],"-m",2)==0) ? atoi(argv[3]+2) : 0;
   int maxRebootDistance  = (argc >= 5 && strncmp(argv[4],"-r",2)==0) ? atoi(argv[4]+2) : 0;
   int maxSplitIter = (argc >= 6 && strncmp(argv[5],"-i",2)==0) ? atoi(argv[5]+2) : INT_MAX;

   cout << "N = " << N << endl;   
   cout << "width = " << width << endl;   
   cout << "mode = " << mode << endl;
   cout << "max reboot distance = " << maxRebootDistance << endl;
   cout << "max split iterations = " << maxSplitIter << endl;

   auto start = RuntimeMonitor::cputime();

   CPSolver::Ptr cp  = Factory::makeSolver();
   // auto xVars = Factory::intVarArray(cp, N, 0, N-1);
   // auto yVars = Factory::intVarArray(cp, N-1, 1, N-1);

   auto vars = Factory::intVarArray(cp, 2*N-1, 0, N-1);
   // vars[0] = x[0]
   // vars[1] = x[1]
   // vars[2] = y[1]
   // vars[3] = x[2]
   // vars[4] = y[2]
   // ...
   // vars[i] = x[ ceil(i/2) ] if i is odd
   // vars[i] = y[ i/2 ]       if i is even

   set<int> xVarsIdx = filter(range(0,2*N-2),[](int i) {return i==0 || i%2!=0;});
   set<int> yVarsIdx = filter(range(2,2*N-2),[](int i) {return i%2==0;});

   for (int i=2; i<2*N-1; i+=2) 
      cp->post(vars[i] != 0);

   auto xVars = all(cp, xVarsIdx, [&vars](int i) {return vars[i];});
   auto yVars = all(cp, yVarsIdx, [&vars](int i) {return vars[i];});

   std::cout << "x = " << xVars << endl;
   std::cout << "y = " << yVars << endl;
   
   
   auto mdd = Factory::makeMDDRelax(cp,width,maxRebootDistance,maxSplitIter);
   //mdd->getSpec().useApproximateEquivalence();

   if (mode == 0) {
     cout << "domain encoding with equalAbsDiff constraint" << endl;
     cp->post(Factory::allDifferentAC(xVars));
     cp->post(Factory::allDifferentAC(yVars));
     for (int i=0; i<N-1; i++) {
       cp->post(equalAbsDiff(yVars[i], xVars[i+1], xVars[i]));
     }
   }
   if ((mode == 1) || (mode == 3)) {
      cout << "domain encoding with AbsDiff-Table constraint" << endl;
      cp->post(Factory::allDifferentAC(xVars));
      cp->post(Factory::allDifferentAC(yVars));

      std::vector<std::vector<int>> table;
      for (int i=0; i<N-1; i++) {
         for (int j=i+1; j<N; j++) {
            table.emplace_back(std::vector<int> {i,j,std::abs(i-j)});           
            table.emplace_back(std::vector<int> {j,i,std::abs(i-j)});
         }
      }
      std::cout << table << std::endl;
      auto tmpFirst = all(cp, {0,1,2}, [&vars](int i) {return vars[i];});     
      cp->post(Factory::table(tmpFirst, table));
      for (int i=1; i<N-1; i++) {
         std::set<int> tmpVarsIdx = {2*i-1,2*i+1,2*i+2};       
         auto tmpVars = all(cp, tmpVarsIdx, [&vars](int i) {return vars[i];});
         cp->post(Factory::table(tmpVars, table));       
      }
   }
   if ((mode == 2) || (mode == 3)) {
      cout << "MDD encoding" << endl;     
      mdd->post(Factory::absDiffMDD(mdd,{vars[0],vars[1],vars[2]}));
      for (int i=1; i<N-1; i++) 
         mdd->post(Factory::absDiffMDD(mdd,{vars[2*i-1],vars[2*i+1],vars[2*i+2]}));      
      mdd->post(allDiffMDD(xVars));
      mdd->post(allDiffMDD(yVars));
      cp->post(mdd);
      //mdd->saveGraph();
      cout << "For testing purposes: adding domain consistent AllDiffs MDD encoding" << endl;
      cp->post(allDifferentAC(xVars));
      cp->post(allDifferentAC(yVars));
   }
   if ((mode < 0) || (mode > 3)) {
     cout << "Exit: specify a mode in {0,1,2,3}:" << endl;
     cout << "  0: domain encoding using AbsDiff" << endl;
     cout << "  1: domain encoding using Table" << endl;
     cout << "  2: MDD encoding" << endl;
     cout << "  3: domain (table) + MDD encoding" << endl;
     exit(1);
   }


   DFSearch search(cp,[=]() {
      // Lex order
      auto x = selectFirst(xVars,[](const auto& x) { return x->size() > 1;});
      if (x) {	
         int c = x->min();          
         return  [=] {
           //std::string tabs(cp->getStateManager()->depth(),'\t');
           //cout << tabs <<  "->choose: " << x << " == " << c << endl; 
            cp->post(x == c);
            //cout << tabs << "<-choose: " << x << " == " << c << endl; 
         }     | [=] {
           //std::string tabs(cp->getStateManager()->depth(),'\t');
           //cout << tabs << "->choose: " << x << " != " << c << endl; 
            cp->post(x != c);
            //cout << tabs << "<-choose: " << x << " != " << c << endl; 
         };	
      } else return Branches({});
   });

   int cnt = 0;
   RuntimeMonitor::HRClock firstSolTime = RuntimeMonitor::cputime();
   int firstSolNumFail = 0;
   SearchStatistics stat;

   search.onSolution([&]() {
      cnt++;
      std::cout << "\rNumber of solutions:" << cnt; 
      // std::cout << "x = " << xVars << "\n";
      // std::cout << "y = " << yVars << endl;
      firstSolTime = RuntimeMonitor::cputime();
      if (cnt == 1) {
         firstSolTime = RuntimeMonitor::cputime();
         firstSolNumFail = stat.numberOfFailures();
      }
   });

      
   stat = search.solve([&stat](const SearchStatistics& stats) {
      stat = stats;
      return stats.numberOfSolutions() > INT_MAX;
   });

   auto end = RuntimeMonitor::cputime();
   cout << stat << endl;
   extern int iterMDD;
   extern int splitCS,pruneCS,potEXEC;
   extern int nbCONSCall,nbCONSFail;
   extern int nbAECall,nbAEFail;

   std::cout << "Time : " << RuntimeMonitor::milli(start,end) << '\n';
   std::cout << "I/C  : " << (double)iterMDD/stat.numberOfNodes() << '\n';
   std::cout << "#L   : " << mdd->nbLayers() << '\n';

   extern int splitCS,pruneCS,potEXEC;
   std::cout << "SPLIT:" << splitCS << " \tpruneCS:" << pruneCS << " \tpotEXEC:" << potEXEC << '\n';

   std::cout << "{ \"JSON\" :\n {";
   std::cout << "\n\t\"allInterval\" :" << "{\n";
   std::cout << "\t\t\"size\" : " << N << ",\n";
   std::cout << "\t\t\"m\" : " << mode << ",\n";
   std::cout << "\t\t\"w\" : " << width << ",\n";
   std::cout << "\t\t\"r\" : " << maxRebootDistance << ",\n";
   std::cout << "\t\t\"i\" : " << maxSplitIter << ",\n";
   std::cout << "\t\t\"nodes\" : " << stat.numberOfNodes() << ",\n";
   std::cout << "\t\t\"fails\" : " << stat.numberOfFailures() << ",\n";
   std::cout << "\t\t\"iter\" : " << iterMDD << ",\n";
   std::cout << "\t\t\"layers\" : " << mdd->nbLayers() << ",\n";
   std::cout << "\t\t\"splitCS\" : " << splitCS << ",\n";
   std::cout << "\t\t\"pruneCS\" : " << pruneCS << ",\n";
   std::cout << "\t\t\"pot\" : " << potEXEC << ",\n";  
   std::cout << "\t\t\"time\" : " << RuntimeMonitor::milli(start,end) << "\n";
   std::cout << "\t}\n";  
   std::cout << "}\n}";
}
