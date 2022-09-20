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

MDDRelax::Ptr newMDDRelax(CPSolver::Ptr cp, int relaxSize, int maxRebootDistance, int maxSplitIter, int nodePriorityAggregateStrategy, int candidatePriorityAggregateStrategy, bool useApproxEquiv, bool approxThenExact) {
  auto mdd = new MDDRelax(cp,relaxSize,maxRebootDistance, relaxSize * maxSplitIter, approxThenExact);
  if (useApproxEquiv) {
    mdd->getSpec().useApproximateEquivalence();
  }
  mdd->getSpec().setNodePriorityAggregateStrategy(nodePriorityAggregateStrategy);
  mdd->getSpec().setCandidatePriorityAggregateStrategy(candidatePriorityAggregateStrategy);
  return mdd;
}

template <typename F>
void addMDDConstraint(CPSolver::Ptr cp, MDDRelax::Ptr mdd, int relaxSize, int maxRebootDistance, int maxSplitIter, int nodePriorityAggregateStrategy, int candidatePriorityAggregateStrategy, bool useApproxEquiv, bool approxThenExact, bool sameMDD, F&& constraint) {
  if (!sameMDD) {
    mdd = newMDDRelax(cp, relaxSize, maxRebootDistance, maxSplitIter, nodePriorityAggregateStrategy, candidatePriorityAggregateStrategy, useApproxEquiv, approxThenExact);
  }
  constraint(mdd);
  if (!sameMDD) {
    cp->post(mdd);
  }
}

void buildModel(CPSolver::Ptr cp, int relaxSize, int mode, int maxRebootDistance, int maxSplitIter, int horizonSize, int density, int randSeed, int nodePriority, int nodePriorityAggregateStrategy, int candidatePriority, int candidatePriorityAggregateStrategy, bool useApproxEquiv, bool approxThenExact, int approxEquivMode, int equivalenceThreshold, bool sameMDD)
{

  int H = horizonSize;

  //auto vars = Factory::intVarArray(cp, H, 0, 1); // vars[i]=1 means that the nurse works on day i
  auto vars = Factory::boolVarArray(cp, H); // vars[i]=1 means that the nurse works on day i

  auto objective = Factory::makeIntVar(cp, 0, H*H);

  auto start = RuntimeMonitor::cputime();

  MDDRelax::Ptr mdd = nullptr;

  std::mt19937 rnG(randSeed);
  std::uniform_real_distribution<double> sampler(0,100);

  std::vector<std::vector<int> > weights(H);
  for (int i = 0; i < H; i++) {
     weights[i].resize(H);
  }

  for (int i = 0; i < H-1; i++) {
    for (int j = i + 1; j < H; j++) {
      weights[i][j] = (weights[j][i] = (sampler(rnG) < density ? (floor(sampler(rnG)/33.33) - 1) : 0));
      std::cout << "Weight (" << i << "," << j << ") = " << weights[i][j] << "\n";
    }
  }

//  cp->post(vars[0] == false);

  if (mode == 0) {
    auto edgeCrosses = Factory::intVarArray(cp, H*H,[&](int x) {
      return xOR(vars[x/H], vars[x%H]);
    });
    for (int i = 0; i < H; i++) {
      for (int j = i + 1; j < H; j++) {
        for (int k = 0; k < H; k++) {
          if (k == i || k == j) continue;
          cp->post(edgeCrosses[i*H + j] + edgeCrosses[i*H + k] + edgeCrosses[j*H + k] <= 2);
          cp->post(edgeCrosses[i*H + j] + edgeCrosses[i*H + k] >= edgeCrosses[j*H + k]);
        }
      }
    }
    auto objSum = Factory::intVarArray(cp,H*H,[&](int x) { 
      int i = x/H;
      int j = x%H;
      if (i < j) return edgeCrosses[x]*weights[i][j];
      return edgeCrosses[x]*0;
    });
    cp->post(objective == Factory::sum(objSum));
  } else if (mode == 1) {
    if (sameMDD) mdd = newMDDRelax(cp, relaxSize, maxRebootDistance, maxSplitIter,
                                   nodePriorityAggregateStrategy, candidatePriorityAggregateStrategy, useApproxEquiv, approxThenExact);

    MDDOpts opts = {
       .nodeP = nodePriority,
       .candP = candidatePriority,
       .cstrP = 0,
       .appxEQMode = approxEquivMode,
       .eqThreshold = equivalenceThreshold
    };
    set<int> allVars;
    for (int i = 0; i < H; i++) allVars.insert(i);
    auto adv = all(cp, allVars, vars);
    addMDDConstraint(cp, mdd, relaxSize, maxRebootDistance, maxSplitIter, nodePriorityAggregateStrategy, candidatePriorityAggregateStrategy, useApproxEquiv, approxThenExact, sameMDD, [adv, weights, objective,opts](MDDRelax::Ptr mdd) { 
       mdd->post(Factory::maxCutObjectiveMDD(mdd, adv, weights, objective,opts));
      //Factory::allDiffMDD(mdd->getSpec(), adv, 0);
    });
  }

  if (mdd && sameMDD) {
//    addMDDConstraint(cp, mdd, relaxSize, maxRebootDistance, maxSplitIter, nodePriorityAggregateStrategy, candidatePriorityAggregateStrategy, useApproxEquiv, approxThenExact, sameMDD, [vars, objective](MDDRelax* mdd) { 
//      Factory::sumMDD(mdd->getSpec(), vars, objective);
//    });
    cp->post(mdd);
  }

  Objective::Ptr obj = Factory::maximize(objective);
  
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
	int c = x->max();
	
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
  int optimalSol = 0;
  search.onSolution([&cnt,&vars,&obj,&optimalSol,&firstSolTime,&firstSolNumFail,&stat,&H]() {
                       std::cout << "obj : " << obj->value() << "\n";
                       optimalSol = obj->value();
                       std::vector<int> inS, inT;
                       for (int i = 0; i < H; i++) {
                         if (vars[i]->isTrue()) inT.push_back(i);
                         else inS.push_back(i);
                       }
                       std::cout << "In S: " << inS << "\n";
                       std::cout << "In T: " << inT << "\n";

                       ++cnt;
                       if (cnt == 1) {
                         firstSolTime = RuntimeMonitor::cputime();
                         firstSolNumFail = stat.numberOfFailures();
                       }
    });
   search.optimize(obj,stat);


//  stat = search.solve([&stat](const SearchStatistics& stats) {
//                              stat = stats;
//                              //return stats.numberOfNodes() > 1;
//                              return stats.numberOfSolutions() > INT_MAX;
//                              //return stats.numberOfSolutions() > 0;
//    }); 
  cout << stat << endl;
  
  auto end = RuntimeMonitor::cputime();
  extern int iterMDD;
  extern int nbCSDown,hitCSDown,nbCSUp,hitCSUp;
  extern int splitCS,pruneCS,potEXEC;
  extern int nbCONSCall,nbCONSFail;
  extern int nbAECall,nbAEFail;
  extern int timeDoingUp, timeDoingDown, timeDoingSplit, timeDoingUpProcess, timeDoingUpFilter;
  
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

  std::cout << "{ \"JSON\" :\n {";
  std::cout << "\n\t\"MCP\" :" << "{\n";
  std::cout << "\t\t\"m\" : " << mode << ",\n";
  std::cout << "\t\t\"w\" : " << relaxSize << ",\n";
  std::cout << "\t\t\"r\" : " << maxRebootDistance << ",\n";
  std::cout << "\t\t\"i\" : " << maxSplitIter << ",\n";
  std::cout << "\t\t\"h\" : " << horizonSize << ",\n";
  std::cout << "\t\t\"d\" : " << density << ",\n";
  std::cout << "\t\t\"r\" : " << randSeed << ",\n";
  std::cout << "\t\t\"n\" : " << nodePriority << ",\n";
  std::cout << "\t\t\"na\" : " << nodePriorityAggregateStrategy << ",\n";
  std::cout << "\t\t\"c\" : " << candidatePriority << ",\n";
  std::cout << "\t\t\"ca\" : " << candidatePriorityAggregateStrategy << ",\n";
  std::cout << "\t\t\"a\" : " << useApproxEquiv << ",\n";
  std::cout << "\t\t\"e\" : " << approxThenExact << ",\n";
  std::cout << "\t\t\"p\" : " << approxEquivMode << ",\n";
  std::cout << "\t\t\"t\" : " << equivalenceThreshold << ",\n";
  std::cout << "\t\t\"j\" : " << sameMDD << ",\n";
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
  std::cout << "\t\t\"solns\" : " << stat.numberOfSolutions() << ",\n";
  std::cout << "\t\t\"optimal\" : " << optimalSol << "\n";
  std::cout << "\t}\n";  
  std::cout << "}\n}\n";
}

int main(int argc,char* argv[])
{
   int width = (argc >= 2 && strncmp(argv[1],"-w",2)==0) ? atoi(argv[1]+2) : 1;
   int mode  = (argc >= 3 && strncmp(argv[2],"-m",2)==0) ? atoi(argv[2]+2) : 1;
   int maxRebootDistance = (argc >= 4 && strncmp(argv[3],"-r",2)==0) ? atoi(argv[3]+2) : INT_MAX;
   int maxSplitIter = (argc >= 5 && strncmp(argv[4],"-i",2)==0) ? atoi(argv[4]+2) : INT_MAX;
   int horizonSize = (argc >= 6 && strncmp(argv[5],"-h",2)==0) ? atoi(argv[5]+2) : 250;
   int density = (argc >= 7 && strncmp(argv[6],"-d",2)==0) ? atoi(argv[6]+2) : 10;
   int randSeed = (argc >= 8 && strncmp(argv[7],"-rand",5)==0) ? atoi(argv[7]+5) : 1;
   int nodePriority = (argc >= 9 && strncmp(argv[8],"-n",2)==0) ? atoi(argv[8]+2) : 0;
   int nodePriorityAggregateStrategy = (argc >= 10 && strncmp(argv[9],"-na",3)==0) ? atoi(argv[9]+3) : 1;
   int candidatePriority = (argc >= 11 && strncmp(argv[10],"-c",2)==0) ? atoi(argv[10]+2) : 0;
   int candidatePriorityAggregateStrategy = (argc >= 12 && strncmp(argv[11],"-ca",3)==0) ? atoi(argv[11]+3) : 1;
   bool useApproxEquiv = (argc >= 13 && strncmp(argv[12],"-a",2)==0) ? atoi(argv[12]+2) : true;
   bool approxThenExact = (argc >= 14 && strncmp(argv[13],"-e",2)==0) ? atoi(argv[13]+2) : true;
   int approxEquivMode = (argc >= 15 && strncmp(argv[14],"-p",2)==0) ? atoi(argv[14]+2) : 0;
   int equivalenceThreshold = (argc >= 16 && strncmp(argv[15],"-t",2)==0) ? atoi(argv[15]+2) : 3;
   bool sameMDD = (argc >= 17 && strncmp(argv[16],"-j",2)==0) ? atoi(argv[16]+2) : true;

   std::cout << "width = " << width << std::endl;
   std::cout << "mode = " << mode << std::endl;
   std::cout << "maxRebootDistance = " << maxRebootDistance << std::endl;
   std::cout << "maxSplitIter = " << maxSplitIter << std::endl;
   std::cout << "horizonSize = " << horizonSize << std::endl;
   std::cout << "density = " << density << std::endl;
   std::cout << "randSeed = " << randSeed << std::endl;
   std::cout << "nodePriority = " << nodePriority << std::endl;
   std::cout << "nodePriorityAggregateStrategy = " << nodePriorityAggregateStrategy << std::endl;
   std::cout << "candidatePriority = " << candidatePriority << std::endl;
   std::cout << "candidatePriorityAggregateStrategy = " << candidatePriorityAggregateStrategy << std::endl;
   std::cout << "useApproxEquiv = " << useApproxEquiv << std::endl;
   std::cout << "approxThenExact = " << approxThenExact << std::endl;
   std::cout << "approxEquivMode = " << approxEquivMode << std::endl;
   std::cout << "equivalenceThreshold = " << equivalenceThreshold << std::endl;
   std::cout << "sameMDD = " << sameMDD << std::endl;
   
   try {
      CPSolver::Ptr cp  = Factory::makeSolver();
      buildModel(cp, width, mode, maxRebootDistance, maxSplitIter, horizonSize, density, randSeed, nodePriority, nodePriorityAggregateStrategy, candidatePriority, candidatePriorityAggregateStrategy, useApproxEquiv, approxThenExact, approxEquivMode, equivalenceThreshold, sameMDD);
   } catch(Status s) {
      std::cout << "model infeasible during post" << std::endl;
   } catch (std::exception& e) {
      std::cerr << e.what() << std::endl;
      std::cerr << "Unable to find the file" << std::endl;
   }

   return 0;   
}
