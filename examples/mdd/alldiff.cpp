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

#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <regex>
#include <fstream>      // std::ifstream
#include <iomanip>
#include <iostream>
#include <set>
#include <tuple>
#include <limits>
#include <iterator>
#include <climits>

#include "solver.hpp"
#include "trailable.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"
#include "mdd.hpp"
#include "RuntimeMonitor.hpp"
#include "mddrelax.hpp"
#include "mddConstraints.hpp"
#include "matrix.hpp"

using namespace Factory;
using namespace std;

template <class Vec>
Vec all(CPSolver::Ptr cp,const set<int>& over,const Vec& t)
{
   Vec res(over.size(),t.get_allocator()); //= Factory::intVarArray(cp, (int) over.size());
   int i = 0;
   for(auto e : over)
      res[i++] = t[e];
   return res;
}

MDDRelax* newMDDRelax(CPSolver::Ptr cp, int relaxSize, int maxRebootDistance, int maxSplitIter,
                      int nodePriorityAggregateStrategy, int candidatePriorityAggregateStrategy,
                      bool useApproxEquiv, bool approxThenExact, int maxPriority)
{
  auto mdd = new MDDRelax(cp,relaxSize,maxRebootDistance, maxSplitIter, approxThenExact, maxPriority);
  if (useApproxEquiv) {
    mdd->getSpec().useApproximateEquivalence();
  }
  mdd->getSpec().setNodePriorityAggregateStrategy(nodePriorityAggregateStrategy);
  mdd->getSpec().setCandidatePriorityAggregateStrategy(candidatePriorityAggregateStrategy);
  return mdd;
}

template <typename F>
void addMDDConstraint(CPSolver::Ptr cp, MDDRelax* mdd, int relaxSize, int maxRebootDistance,
                      int maxSplitIter, int nodePriorityAggregateStrategy,
                      int candidatePriorityAggregateStrategy, bool useApproxEquiv,
                      bool approxThenExact, int maxConstraintPriority, bool sameMDD, F&& constraint)
{
  if (!sameMDD) {
    mdd = newMDDRelax(cp, relaxSize, maxRebootDistance, maxSplitIter, nodePriorityAggregateStrategy,
                      candidatePriorityAggregateStrategy, useApproxEquiv, approxThenExact,
                      maxConstraintPriority);
  }
  constraint(mdd);
  if (!sameMDD) {
    cp->post(mdd);
  }
}

void buildModel(CPSolver::Ptr cp, int relaxSize, int mode, int maxRebootDistance,
                int maxSplitIter, int numVariables, int maxDomainSize, int nodePriority,
                int nodePriorityAggregateStrategy, int candidatePriority,
                int candidatePriorityAggregateStrategy, bool useApproxEquiv, bool approxThenExact,
                int approxEquivMode, int equivalenceThreshold, int constraintType1Priority,
                int constraintType2Priority, int constraintType3Priority, int constraintType4Priority,
                int constraintType5Priority, bool sameMDD, int randSeed)
{
  int maxConstraintPriority = std::max(std::max(std::max(constraintType1Priority, constraintType2Priority),
                                                std::max(constraintType3Priority, constraintType4Priority)),
                                       constraintType5Priority);

  int sizes[5] =         {  3,   6, 10,  8, 20};
  int frequencies[5] =   {  1,   6,  1,  5,  7};
  int offsets[5] =       {  0,   0,  0,  0,  0};
  int inclusionProb[5] = {100, 100, 30, 60, 20};
  int priorities[5] = {constraintType1Priority, constraintType2Priority,
                       constraintType3Priority, constraintType4Priority, constraintType5Priority};

  std::mt19937 rnG(randSeed);
  std::uniform_real_distribution<double> sampler(0,100);

  std::vector< std::vector< set<int> > > cliquesByConstraint;

  int largestCliqueSize = 0;

  for (int constraintIndex = 0; constraintIndex < 5; constraintIndex++) {
    std::vector< set<int> > cliques;
    for (int cliqueIndex = 0; cliqueIndex < numVariables/frequencies[constraintIndex]; cliqueIndex++) {
      int firstVar = cliqueIndex * frequencies[constraintIndex] + offsets[constraintIndex];
      int lastVar = firstVar + sizes[constraintIndex] - 1;
      if (lastVar < numVariables) {
        set<int> clique;
        do {
          clique = set<int>();
          for (int varIndex = firstVar; varIndex <= lastVar; varIndex++) { 
             if (sampler(rnG) < inclusionProb[constraintIndex]) clique.insert(varIndex);
          }
        } while (clique.size() <= 1 || (int)clique.size() > maxDomainSize);
        if ((int)clique.size() > largestCliqueSize) largestCliqueSize = clique.size();
        cliques.push_back(clique);
        for (int val : clique)
          std::cout << val << " ";
        std::cout << "\n";
      } else break;
    }
    cliquesByConstraint.push_back(cliques);
  }

  std::cout << "Domain size: " << largestCliqueSize << "\n";
  auto vars = Factory::intVarArray(cp, numVariables, 1, largestCliqueSize);

  cp->post(vars[0] == 1);
  cp->post(vars[1] == 2);
  cp->post(vars[2] == 3);
  cp->post(vars[3] == 4);
  cp->post(vars[4] == 5);
  cp->post(vars[5] == 6);

  auto start = RuntimeMonitor::cputime();
  MDDRelax* mdd = nullptr;
  try {
    if ((mode == 1 || mode == 2) && sameMDD) {
      mdd = newMDDRelax(cp, relaxSize, maxRebootDistance, maxSplitIter, nodePriorityAggregateStrategy, candidatePriorityAggregateStrategy, useApproxEquiv, approxThenExact, maxConstraintPriority);
    }

    if (mode == 0 || mode == 2) {
      cout << "Classic" << endl;
      for (int constraintIndex = 0; constraintIndex < 5; constraintIndex++) {
        for (int cliqueIndex = 0; cliqueIndex < (int)cliquesByConstraint[constraintIndex].size(); cliqueIndex++) { 
          auto adv = all(cp, cliquesByConstraint[constraintIndex][cliqueIndex], vars);
          cp->post(Factory::allDifferentAC(adv));
        }
      }
    }
    if (mode == 1 || mode == 2) {
      cout << "AllDiff MDD" << endl;
      for (int constraintIndex = 0; constraintIndex < 5; constraintIndex++) {
        for (int cliqueIndex = 0; cliqueIndex < (int)cliquesByConstraint[constraintIndex].size(); cliqueIndex++) { 
          auto adv = all(cp, cliquesByConstraint[constraintIndex][cliqueIndex], vars);
          addMDDConstraint(cp, mdd, relaxSize, maxRebootDistance, maxSplitIter, nodePriorityAggregateStrategy, candidatePriorityAggregateStrategy, useApproxEquiv, approxThenExact, maxConstraintPriority, sameMDD, [adv, nodePriority, candidatePriority, approxEquivMode, equivalenceThreshold, priorities, constraintIndex](MDDRelax* mdd) {
            Factory::allDiffMDD2(mdd->getSpec(), adv, nodePriority, candidatePriority, approxEquivMode, equivalenceThreshold, priorities[constraintIndex]);
          });
        }
      }
    }

    if (sameMDD && mdd) {
      cp->post(mdd);
    }
  } catch(Status s) {
     auto end = RuntimeMonitor::cputime();
     std::cout << "model infeasible during post" << std::endl;
     std::cout << "Time : " << RuntimeMonitor::milli(start,end) << '\n';
  extern int iterMDD;
  extern int nbCSDown,hitCSDown,nbCSUp,hitCSUp;
  extern int splitCS,pruneCS,potEXEC;
  extern int nbCONSCall,nbCONSFail;
  extern int nbAECall,nbAEFail;
  extern int fullReboot, partialReboot;
  std::cout << "Full Reboot: " << fullReboot << "\n";
  std::cout << "Partial Reboot: " << partialReboot << "\n";
  std::cout << "{ \"JSON\" :\n {";
  std::cout << "\n\t\"alldiff\" :" << "{\n";
  std::cout << "\t\t\"m\" : " << mode << ",\n";
  std::cout << "\t\t\"w\" : " << relaxSize << ",\n";
  std::cout << "\t\t\"r\" : " << maxRebootDistance << ",\n";
  std::cout << "\t\t\"i\" : " << maxSplitIter << ",\n";
  std::cout << "\t\t\"h\" : " << numVariables << ",\n";
  std::cout << "\t\t\"s\" : " << maxDomainSize << ",\n";
  std::cout << "\t\t\"n\" : " << nodePriority << ",\n";
  std::cout << "\t\t\"na\" : " << nodePriorityAggregateStrategy << ",\n";
  std::cout << "\t\t\"d\" : " << candidatePriority << ",\n";
  std::cout << "\t\t\"ca\" : " << candidatePriorityAggregateStrategy << ",\n";
  std::cout << "\t\t\"a\" : " << useApproxEquiv << ",\n";
  std::cout << "\t\t\"e\" : " << approxThenExact << ",\n";
  std::cout << "\t\t\"p\" : " << approxEquivMode << ",\n";
  std::cout << "\t\t\"t\" : " << equivalenceThreshold << ",\n";
  std::cout << "\t\t\"t1p\" : " << constraintType1Priority << ",\n";
  std::cout << "\t\t\"t2p\" : " << constraintType2Priority << ",\n";
  std::cout << "\t\t\"t3p\" : " << constraintType3Priority << ",\n";
  std::cout << "\t\t\"t4p\" : " << constraintType4Priority << ",\n";
  std::cout << "\t\t\"t5p\" : " << constraintType5Priority << ",\n";
  std::cout << "\t\t\"j\" : " << sameMDD << ",\n";
  std::cout << "\t\t\"rand\" : " << randSeed << ",\n";
  std::cout << "\t\t\"nodes\" : " << 0 << ",\n";
  std::cout << "\t\t\"fails\" : " << 0 << ",\n";
  std::cout << "\t\t\"iter\" : " << iterMDD << ",\n";
  std::cout << "\t\t\"nbCSDown\" : " << nbCSDown << ",\n";
  if (mdd) std::cout << "\t\t\"layers\" : " << mdd->nbLayers() << ",\n";
  std::cout << "\t\t\"splitCS\" : " << splitCS << ",\n";
  std::cout << "\t\t\"pruneCS\" : " << pruneCS << ",\n";
  std::cout << "\t\t\"pot\" : " << potEXEC << ",\n";  
  std::cout << "\t\t\"time\" : " << RuntimeMonitor::milli(start,end) << ",\n";
  std::cout << "\t\t\"solns\" : " << 0 << "\n";
  std::cout << "\t}\n";  
  std::cout << "}\n}\n";
     return;
  }
  
  
  DFSearch search(cp,[=]() {
      unsigned i = 0u;
      for(i=0u;i < vars.size();i++)
	if (vars[i]->size()> 1) break;
      auto x = i< vars.size() ? vars[i] : nullptr;

      // auto x = selectMin(vars,
      // 			 [](const auto& x) { return x->size() > 1;},
      // 			 [](const auto& x) { return x->size();});
      
      if (x) {
	//bool c = x->min();
	int c = x->min();
	
	return  [=] {
	  cp->post(x == c);}
	| [=] {
	  cp->post(x != c);};
      } else return Branches({});
    });

  int cnt = 0;
  RuntimeMonitor::HRClock firstSolTime;

  SearchStatistics stat;
  int firstSolNumFail = 0;

  search.onSolution([&cnt,&firstSolTime,&firstSolNumFail,&stat]() {
                       ++cnt;
                       if (cnt == 1) {
                         firstSolTime = RuntimeMonitor::cputime();
                         firstSolNumFail = stat.numberOfFailures();
                       }
                       if (cnt % 10000 == 0)
                          std::cout << cnt << "\n";
  });


  stat = search.solve([&stat](const SearchStatistics& stats) {
                              stat = stats;
                              //return stats.numberOfNodes() > 1;
                              //return stats.numberOfSolutions() > INT_MAX;
                              return stats.numberOfSolutions() > 0;
    }); 
  cout << stat << endl;
  
  auto end = RuntimeMonitor::cputime();
  extern int iterMDD;
  extern int nbCSDown,hitCSDown,nbCSUp,hitCSUp;
  extern int splitCS,pruneCS,potEXEC;
  extern int nbCONSCall,nbCONSFail;
  extern int nbAECall,nbAEFail;
  extern int timeDoingUp, timeDoingDown, timeDoingSplit, timeDoingUpProcess, timeDoingUpFilter;
  extern int fullReboot, partialReboot;
  
  std::cout << "Time : " << RuntimeMonitor::milli(start,end) << '\n';
  std::cout << "I/C  : " << (double)iterMDD/stat.numberOfNodes() << '\n';
  std::cout << "#CS (Down) : " << nbCSDown << '\n';
  std::cout << "#CS (Up) : " << nbCSUp << '\n';
  if (mdd) std::cout << "#L   : " << mdd->nbLayers() << '\n';
  std::cout << "SPLIT:" << splitCS << " \tpruneCS:" << pruneCS << " \tpotEXEC:" << potEXEC << '\n';
  std::cout << "CONS:" << nbCONSCall << " \tFAIL:" << nbCONSFail << '\n';
  std::cout << "NBAE:" << nbAECall << " \tAEFAIL:" << nbAEFail << '\n';
  std::cout << "HIT (DOWN): " << (double)hitCSDown /nbCSDown << '\n';
  std::cout << "HIT (UP)  : " << (double)hitCSUp /nbCSUp << '\n';

  std::cout << "Time Doing Down: " << timeDoingDown << "\n";
  std::cout << "Time Doing Up: " << timeDoingUp << "\n";
  std::cout << "      Process: " << timeDoingUpProcess << "\n";
  std::cout << "       Filter: " << timeDoingUpFilter << "\n";
  std::cout << "Time Doing Split: " << timeDoingSplit << "\n";

  std::cout << "Full Reboot: " << fullReboot << "\n";
  std::cout << "Partial Reboot: " << partialReboot << "\n";

  std::cout << "{ \"JSON\" :\n {";
  std::cout << "\n\t\"alldiff\" :" << "{\n";
  std::cout << "\t\t\"m\" : " << mode << ",\n";
  std::cout << "\t\t\"w\" : " << relaxSize << ",\n";
  std::cout << "\t\t\"r\" : " << maxRebootDistance << ",\n";
  std::cout << "\t\t\"i\" : " << maxSplitIter << ",\n";
  std::cout << "\t\t\"h\" : " << numVariables << ",\n";
  std::cout << "\t\t\"s\" : " << maxDomainSize << ",\n";
  std::cout << "\t\t\"n\" : " << nodePriority << ",\n";
  std::cout << "\t\t\"na\" : " << nodePriorityAggregateStrategy << ",\n";
  std::cout << "\t\t\"d\" : " << candidatePriority << ",\n";
  std::cout << "\t\t\"ca\" : " << candidatePriorityAggregateStrategy << ",\n";
  std::cout << "\t\t\"a\" : " << useApproxEquiv << ",\n";
  std::cout << "\t\t\"e\" : " << approxThenExact << ",\n";
  std::cout << "\t\t\"p\" : " << approxEquivMode << ",\n";
  std::cout << "\t\t\"t\" : " << equivalenceThreshold << ",\n";
  std::cout << "\t\t\"t1p\" : " << constraintType1Priority << ",\n";
  std::cout << "\t\t\"t2p\" : " << constraintType2Priority << ",\n";
  std::cout << "\t\t\"t3p\" : " << constraintType3Priority << ",\n";
  std::cout << "\t\t\"t4p\" : " << constraintType4Priority << ",\n";
  std::cout << "\t\t\"t5p\" : " << constraintType5Priority << ",\n";
  std::cout << "\t\t\"j\" : " << sameMDD << ",\n";
  std::cout << "\t\t\"rand\" : " << randSeed << ",\n";
  std::cout << "\t\t\"nodes\" : " << stat.numberOfNodes() << ",\n";
  std::cout << "\t\t\"fails\" : " << stat.numberOfFailures() << ",\n";
  std::cout << "\t\t\"iter\" : " << iterMDD << ",\n";
  std::cout << "\t\t\"nbCSDown\" : " << nbCSDown << ",\n";
  if (mdd) std::cout << "\t\t\"layers\" : " << mdd->nbLayers() << ",\n";
  std::cout << "\t\t\"splitCS\" : " << splitCS << ",\n";
  std::cout << "\t\t\"pruneCS\" : " << pruneCS << ",\n";
  std::cout << "\t\t\"pot\" : " << potEXEC << ",\n";  
  std::cout << "\t\t\"time\" : " << RuntimeMonitor::milli(start,end) << ",\n";
  std::cout << "\t\t\"timeToFirstSol\" : " << RuntimeMonitor::milli(start,firstSolTime) << ",\n";
  std::cout << "\t\t\"failsForFirstSol\" : " << firstSolNumFail << ",\n";
  std::cout << "\t\t\"solns\" : " << stat.numberOfSolutions() << "\n";
  std::cout << "\t}\n";  
  std::cout << "}\n}\n";
}

int main(int argc,char* argv[])
{
   int width = (argc >= 2 && strncmp(argv[1],"-w",2)==0) ? atoi(argv[1]+2) : 1;
   int mode  = (argc >= 3 && strncmp(argv[2],"-m",2)==0) ? atoi(argv[2]+2) : 1;
   int maxRebootDistance = (argc >= 4 && strncmp(argv[3],"-r",2)==0) ? atoi(argv[3]+2) : INT_MAX;
   int maxSplitIter = (argc >= 5 && strncmp(argv[4],"-i",2)==0) ? atoi(argv[4]+2) : INT_MAX;
   int numVariables = (argc >= 6 && strncmp(argv[5],"-h",2)==0) ? atoi(argv[5]+2) : 50;
   int maxDomainSize = (argc >= 7 && strncmp(argv[6],"-s",2)==0) ? atoi(argv[6]+2) : 7;
   int nodePriority = (argc >= 8 && strncmp(argv[7],"-n",2)==0) ? atoi(argv[7]+2) : 0;
   int nodePriorityAggregateStrategy = (argc >= 9 && strncmp(argv[8],"-na",3)==0) ? atoi(argv[8]+3) : 1;
   int candidatePriority = (argc >= 10 && strncmp(argv[9],"-d",2)==0) ? atoi(argv[9]+2) : 0;
   int candidatePriorityAggregateStrategy = (argc >= 11 && strncmp(argv[10],"-ca",3)==0) ? atoi(argv[10]+3) : 1;
   bool useApproxEquiv = (argc >= 12 && strncmp(argv[11],"-a",2)==0) ? atoi(argv[11]+2) : true;
   bool approxThenExact = (argc >= 13 && strncmp(argv[12],"-e",2)==0) ? atoi(argv[12]+2) : true;
   int approxEquivMode = (argc >= 14 && strncmp(argv[13],"-p",2)==0) ? atoi(argv[13]+2) : 0;
   int equivalenceThreshold = (argc >= 15 && strncmp(argv[14],"-t",2)==0) ? atoi(argv[14]+2) : 4;
   int constraintType1Priority = (argc >= 16 && strncmp(argv[15],"-t1p",4)==0) ? atoi(argv[15]+4) : 0;
   int constraintType2Priority = (argc >= 17 && strncmp(argv[16],"-t2p",4)==0) ? atoi(argv[16]+4) : 0;
   int constraintType3Priority = (argc >= 18 && strncmp(argv[17],"-t3p",4)==0) ? atoi(argv[17]+4) : 0;
   int constraintType4Priority = (argc >= 19 && strncmp(argv[18],"-t4p",4)==0) ? atoi(argv[18]+4) : 0;
   int constraintType5Priority = (argc >= 20 && strncmp(argv[19],"-t5p",4)==0) ? atoi(argv[19]+4) : 0;
   bool sameMDD = (argc >= 21 && strncmp(argv[20],"-j",2)==0) ? atoi(argv[20]+2) : true;
   int randSeed = (argc >= 22 && strncmp(argv[21],"-rand",5)==0) ? atoi(argv[21]+5) : 0;

   // mode: 0 (Classic), 1 (AllDiff MDD)
   // mode: 2 (AllDiff MDD w/ Classic)
   
   std::cout << "width = " << width << std::endl;
   std::cout << "mode = " << mode << std::endl;
   std::cout << "maxRebootDistance = " << maxRebootDistance << std::endl;
   std::cout << "maxSplitIter = " << maxSplitIter << std::endl;
   std::cout << "numVariables = " << numVariables << std::endl;
   std::cout << "maxDomainSize = " << maxDomainSize << std::endl;
   std::cout << "nodePriority = " << nodePriority << std::endl;
   std::cout << "nodePriorityAggregateStrategy = " << nodePriorityAggregateStrategy << std::endl;
   std::cout << "candidatePriority = " << candidatePriority << std::endl;
   std::cout << "candidatePriorityAggregateStrategy = " << candidatePriorityAggregateStrategy << std::endl;
   std::cout << "useApproxEquiv = " << useApproxEquiv << std::endl;
   std::cout << "approxThenExact = " << approxThenExact << std::endl;
   std::cout << "approxEquivMode = " << approxEquivMode << std::endl;
   std::cout << "equivalenceThreshold = " << equivalenceThreshold << std::endl;
   std::cout << "constraintType1Priority = " << constraintType1Priority << std::endl;
   std::cout << "constraintType2Priority = " << constraintType2Priority << std::endl;
   std::cout << "constraintType3Priority = " << constraintType3Priority << std::endl;
   std::cout << "constraintType4Priority = " << constraintType4Priority << std::endl;
   std::cout << "constraintType5Priority = " << constraintType5Priority << std::endl;
   std::cout << "sameMDD = " << sameMDD << std::endl;
   std::cout << "randSeed = " << randSeed << std::endl;
   
   CPSolver::Ptr cp  = Factory::makeSolver();
   buildModel(cp, width, mode, maxRebootDistance, maxSplitIter, numVariables, maxDomainSize, nodePriority, nodePriorityAggregateStrategy, candidatePriority, candidatePriorityAggregateStrategy, useApproxEquiv, approxThenExact, approxEquivMode, equivalenceThreshold, constraintType1Priority, constraintType2Priority, constraintType3Priority, constraintType4Priority, constraintType5Priority, sameMDD, randSeed);

   return 0;   
}
