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

class Instance
{
   private :
   int _numNodes;
   int _numEdges;
   std::vector< std::set<int> > _cliques;
public:
   Instance(int nodes,int edges,std::vector< std::set<int> >& cliques)
      : _numNodes(nodes), _numEdges(edges), _cliques(cliques) {}
   int numNodes() const { return _numNodes;}
   int numEdges() const { return _numEdges;}
   std::vector< std::set<int> > cliques() const { return _cliques;}
   static Instance readData(const char* filename);
};

void cleanRows(std::string& buf)
{
   buf = std::regex_replace(buf,std::regex("\r"),"\n");
   buf = std::regex_replace(buf,std::regex("\n\n"),"\n");
}

template<typename T> vector<T> split(const std::string& str,char d, std::function<T(const std::string&)> clo)
{
   auto result = vector<T>{};
   auto ss = std::stringstream{str};
   for (std::string line; std::getline(ss, line, d);)
      if(!line.empty())
         result.push_back(clo(line));

   return result;
}

vector<std::string> split(const std::string& str,char d)
{
   return split<std::string>(str,d,[] (const auto& line) -> std::string {return line;});
}

Instance Instance::readData(const char* filename)
{
   std::string buffer;
   std::ifstream t(filename);
   t.seekg(0, std::ios::end);
   if (t.tellg() > 0){
      buffer.reserve(t.tellg());
      t.seekg(0, std::ios::beg);
      buffer.assign((std::istreambuf_iterator<char>(t)),
                    std::istreambuf_iterator<char>());
      cleanRows(buffer);
      auto lines = split(buffer,'\n');
      int lineIndex = 0;
      while (lines[lineIndex][0] == 'c') lineIndex++;
      auto stats = split(lines[lineIndex],' ');
      int numNodes = std::stoi(stats[2]);
      int numEdges = std::stoi(stats[3]);
      lineIndex++;
      int* degrees = new int[numNodes];
      bool** adjacencies = new bool*[numNodes];
      for (int i = 0; i < numNodes; i++) {
         adjacencies[i] = new bool[numNodes];
      }
      for (int i = 0; i < numNodes; i++) {
         degrees[i] = 0;
         for (int j = 0; j < numNodes; j++)
            adjacencies[i][j] = (adjacencies[j][i] = false);
      }
      for (int i = 0; i < numEdges; i++) {
         auto edge = split(lines[lineIndex],' ');
         int node1 = std::stoi(edge[1]) - 1;
         int node2 = std::stoi(edge[2]) - 1;
         adjacencies[node1][node2] = true;
         adjacencies[node2][node1] = true;
         degrees[node1]++;
         degrees[node2]++;
         lineIndex++;
      }

      std::vector< std::set<int> > cliques;
      int remainingEdges = numEdges;
      while (remainingEdges) {
        std::set<int> clique;

        int vertex;
        int vertexDegree;
        while (true) {
          vertex = -1;
          vertexDegree = -1;
          for (int i = 0; i < numNodes; i++) {
            if (clique.find(i) != clique.end()) continue;
            if (degrees[i] > vertexDegree) {
              bool adjacentToAllInClique = true;
              for (int v : clique) {
                if (!adjacencies[i][v]) {
                  adjacentToAllInClique = false;
                  break;
                }
              }
              if (adjacentToAllInClique) {
                vertex = i;
                vertexDegree = degrees[i];
              }
            }
          }
          if (vertex == -1) break;
          clique.insert(vertex);
        }

        cliques.push_back(clique);
        std::cout << "Clique:";
        for (int v1 : clique) {
          std::cout << " " << v1;
          for (int v2 : clique) {
            if (v1 < v2) {
              adjacencies[v1][v2] = false;
              adjacencies[v2][v1] = false;
              degrees[v1]--;
              degrees[v2]--;
              remainingEdges--;
            }
          }
        }
        std::cout << "\n";
      }
      return Instance(numNodes,numEdges,cliques);
   }else
      throw;
}

template <class Vec>
Vec all(CPSolver::Ptr cp,const set<int>& over,const Vec& t)
{
   Vec res(over.size(),t.get_allocator()); //= Factory::intVarArray(cp, (int) over.size());
   int i = 0;
   for(auto e : over)
      res[i++] = t[e];
   return res;
}

MDDRelax* newMDDRelax(CPSolver::Ptr cp, int relaxSize, int maxRebootDistance, int maxSplitIter, int nodePriorityAggregateStrategy, int candidatePriorityAggregateStrategy, bool useApproxEquiv, bool approxThenExact) {
  auto mdd = new MDDRelax(cp,relaxSize,maxRebootDistance, relaxSize * maxSplitIter, approxThenExact);
  if (useApproxEquiv) {
    mdd->getSpec().useApproximateEquivalence();
  }
  mdd->getSpec().setNodePriorityAggregateStrategy(nodePriorityAggregateStrategy);
  mdd->getSpec().setCandidatePriorityAggregateStrategy(candidatePriorityAggregateStrategy);
  return mdd;
}

template <typename F>
void addMDDConstraint(CPSolver::Ptr cp, MDDRelax* mdd, int relaxSize, int maxRebootDistance, int maxSplitIter, int nodePriorityAggregateStrategy, int candidatePriorityAggregateStrategy, bool useApproxEquiv, bool approxThenExact, bool sameMDD, F&& constraint) {
  if (!sameMDD) {
    mdd = newMDDRelax(cp, relaxSize, maxRebootDistance, maxSplitIter, nodePriorityAggregateStrategy, candidatePriorityAggregateStrategy, useApproxEquiv, approxThenExact);
  }
  constraint(mdd);
  if (!sameMDD) {
    cp->post(mdd);
  }
}

void buildModel(CPSolver::Ptr cp, int relaxSize, int mode, int maxRebootDistance, int maxSplitIter, Instance instance, int nodePriority, int nodePriorityAggregateStrategy, int candidatePriority, int candidatePriorityAggregateStrategy, bool useApproxEquiv, bool approxThenExact, int approxEquivMode, int equivalenceThreshold, bool sameMDD)
{

  int H = instance.numNodes();
  std::vector< std::set<int> > cliques = instance.cliques();

  //auto vars = Factory::intVarArray(cp, H, 0, 1); // vars[i]=1 means that the nurse works on day i
  auto vars = Factory::boolVarArray(cp, H); // vars[i]=1 means that the nurse works on day i

  auto objective = Factory::makeIntVar(cp, 0, H);

  std::set<int> included = {1};

  auto start = RuntimeMonitor::cputime();

  MDDRelax* mdd = nullptr;
  MDDOpts opts = {
     .nodeP = nodePriority,
     .candP = candidatePriority,
     .cstrP = 0,
     .appxEQMode = approxEquivMode,
     .eqThreshold = equivalenceThreshold
  };

  if (mode == 0) {
    for (auto clique : cliques) {
      auto adv = all(cp, clique, vars);
      mdd->post(Factory::upToOneMDD(mdd, adv, included, opts));
    }
  } else if (mode == 1) {
    cout << "amongMDD2 encoding" << endl;
    if (sameMDD) mdd = newMDDRelax(cp, relaxSize, maxRebootDistance, maxSplitIter, nodePriorityAggregateStrategy, candidatePriorityAggregateStrategy, useApproxEquiv, approxThenExact);

            
    for (auto clique : cliques) {
      auto adv = all(cp, clique, vars);
      addMDDConstraint(cp, mdd, relaxSize, maxRebootDistance, maxSplitIter, nodePriorityAggregateStrategy, candidatePriorityAggregateStrategy, useApproxEquiv, approxThenExact, sameMDD, [adv, included,opts](MDDRelax* mdd) { 
         mdd->post(Factory::amongMDD2(mdd, adv, 0, 1, included,opts));
      });
    }
  } else if (mode == 2) {
    cout << "upToOneMDD encoding" << endl;
    if (sameMDD) mdd = newMDDRelax(cp, relaxSize, maxRebootDistance, maxSplitIter, nodePriorityAggregateStrategy, candidatePriorityAggregateStrategy, useApproxEquiv, approxThenExact);

    for (auto clique : cliques) {
      auto adv = all(cp, clique, vars);
      addMDDConstraint(cp, mdd, relaxSize, maxRebootDistance, maxSplitIter, nodePriorityAggregateStrategy, candidatePriorityAggregateStrategy, useApproxEquiv, approxThenExact, sameMDD, [adv, included,opts](MDDRelax* mdd) { 
         mdd->post(Factory::upToOneMDD(mdd, adv, included,opts));
      });
    }
  }

  if (mdd && sameMDD) {
    addMDDConstraint(cp, mdd, relaxSize, maxRebootDistance, maxSplitIter, nodePriorityAggregateStrategy, candidatePriorityAggregateStrategy, useApproxEquiv, approxThenExact, sameMDD, [vars, objective](MDDRelax* mdd) { 
       mdd->post(Factory::sum(mdd, vars, objective));
    });
    cp->post(mdd);
  }
  
  Objective::Ptr obj = Factory::maximize(objective);
  
  DFSearch search(cp,[=]() {
//auto current = RuntimeMonitor::cputime();
//if (RuntimeMonitor::milli(start,current) >= 180000) {
//  std::cout << "Taken " << RuntimeMonitor::milli(start,current)/1000 << " seconds.  Lowerbound is " << objective->min() << ".  Upperbound is " << objective->max() << "\n";
//}
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
  search.onSolution([&cnt,&vars,&obj,&firstSolTime,&firstSolNumFail,&stat]() {
                       std::cout << "obj : " << obj->value() << "\n";
                       cout << "Assignment: " << vars << endl;
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
  std::cout << "\n\t\"amongNurse\" :" << "{\n";
  std::cout << "\t\t\"m\" : " << mode << ",\n";
  std::cout << "\t\t\"w\" : " << relaxSize << ",\n";
  std::cout << "\t\t\"r\" : " << maxRebootDistance << ",\n";
  std::cout << "\t\t\"i\" : " << maxSplitIter << ",\n";
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
   const char* fileName = (argc >= 6 && strncmp(argv[5],"-f",2)==0) ? argv[5]+2 : "data/brock200_1.clq";
   int nodePriority = (argc >= 7 && strncmp(argv[6],"-n",2)==0) ? atoi(argv[6]+2) : 0;
   int nodePriorityAggregateStrategy = (argc >= 8 && strncmp(argv[7],"-na",3)==0) ? atoi(argv[7]+3) : 1;
   int candidatePriority = (argc >= 9 && strncmp(argv[8],"-c",2)==0) ? atoi(argv[8]+2) : 0;
   int candidatePriorityAggregateStrategy = (argc >= 10 && strncmp(argv[9],"-ca",3)==0) ? atoi(argv[9]+3) : 1;
   bool useApproxEquiv = (argc >= 11 && strncmp(argv[10],"-a",2)==0) ? atoi(argv[10]+2) : true;
   bool approxThenExact = (argc >= 12 && strncmp(argv[11],"-e",2)==0) ? atoi(argv[11]+2) : true;
   int approxEquivMode = (argc >= 13 && strncmp(argv[12],"-p",2)==0) ? atoi(argv[12]+2) : 0;
   int equivalenceThreshold = (argc >= 14 && strncmp(argv[13],"-t",2)==0) ? atoi(argv[13]+2) : 3;
   bool sameMDD = (argc >= 15 && strncmp(argv[14],"-j",2)==0) ? atoi(argv[14]+2) : true;

   std::cout << "width = " << width << std::endl;
   std::cout << "mode = " << mode << std::endl;
   std::cout << "maxRebootDistance = " << maxRebootDistance << std::endl;
   std::cout << "maxSplitIter = " << maxSplitIter << std::endl;
   std::cout << "fileName = " << fileName << std::endl;
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
      Instance instance = Instance::readData(fileName);
      CPSolver::Ptr cp  = Factory::makeSolver();
      buildModel(cp, width, mode, maxRebootDistance, maxSplitIter, instance, nodePriority, nodePriorityAggregateStrategy, candidatePriority, candidatePriorityAggregateStrategy, useApproxEquiv, approxThenExact, approxEquivMode, equivalenceThreshold, sameMDD);
   } catch(Status s) {
      std::cout << "model infeasible during post" << std::endl;
   } catch (std::exception& e) {
      std::cerr << e.what() << std::endl;
      std::cerr << "Unable to find the file" << std::endl;
   }

   return 0;   
}
