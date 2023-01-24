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
#include <fstream>
#include <regex>
#include <sstream>
#include "solver.hpp"
#include "trailable.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"
#include "mdd.hpp"
#include "mddrelax.hpp"
#include "mddConstraints.hpp"
#include "RuntimeMonitor.hpp"

using namespace std;
using namespace Factory;

void cleanRows(std::string& buf)
{
   buf = std::regex_replace(buf,std::regex("\r"),"\n");
   buf = std::regex_replace(buf,std::regex("\n\n"),"\n");
}

template<typename T> vector<T> split(const std::string& str,char d, std::function<T(const std::string&)> clo, bool removeHeader)
{
   auto result = vector<T>{};
   auto ss = std::stringstream{str};
   for (std::string line; std::getline(ss, line, d);){
      if(removeHeader){
         removeHeader = false;
         continue;
      }
      if(!line.empty())
         result.push_back(clo(line));
   }
   return result;
}
vector<std::string> split(const std::string& str,char d)
{
   return split<std::string>(str,d,[] (const auto& line) -> std::string {return line;}, false);
}
vector<int> splitAsInt(const std::string& str,char d, bool header)
{
   return split<int>(str,d,[] (const auto& s) -> int {return std::stoi(s);}, header);
}
vector<string> readData(const char* filename)
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
      return lines;
   }else
      throw;
}

void buildModel(int numVars, vector<vector<int>> matrix, int mode, int width, int maxSplitIter, int maxRebootDistance, int nodePriorityMode, int nodePriorityAggregateStrategy, int candidatePriorityMode, int candidatePriorityAggregateStrategy, int approxThenExact, int allDiffPriority, int tspPriority, int precedencePriority, int allDiffApproxEquivMode, int tspApproxEquivMode, int precedenceApproxEquivMode) {
   MDDOpts allDiffOpts = {
      .nodeP = 0,
      .candP = 0,
      .cstrP = allDiffPriority,
      .appxEQMode = allDiffApproxEquivMode,
      .eqThreshold = 3
   };
   MDDOpts tspOpts = {
      .nodeP = nodePriorityMode,
      .candP = candidatePriorityMode,
      .cstrP = tspPriority,
      .appxEQMode = tspApproxEquivMode,
      .eqThreshold = 0
   };
   MDDOpts precedenceOpts = {
      .nodeP = nodePriorityMode,
      .candP = candidatePriorityMode,
      .cstrP = precedencePriority,
      .appxEQMode = precedenceApproxEquivMode,
      .eqThreshold = 0
   };
   int maxConstraintPriority = std::max(allDiffPriority, std::max(tspPriority, precedencePriority));

   CPSemSolver::Ptr cp  = Factory::makeSemSolver();
   auto vars = Factory::intVarArray(cp, numVars, 0, numVars - 1);
   int zUB = 0;
   for (auto line : matrix) {
      int maxOnLine = 0;
      for (auto val : line) {
         maxOnLine = std::max(maxOnLine, val);
      }
      zUB += maxOnLine;
   }
   auto z = Factory::makeIntVar(cp, 0, zUB);
   Objective::Ptr obj = Factory::minimize(z);

   cp->post(vars[0] == 0);
   cp->post(vars[numVars - 1] == numVars - 1);
   MDDRelax::Ptr mdd = nullptr;
   if (mode == 0) {
      cout << "Domain encoding" << endl;
   } else if (mode == 1) {
      cout << "MDD encoding w/ Precedences" << endl;
      mdd = new MDDRelax(cp,width,maxRebootDistance,maxSplitIter,approxThenExact,maxConstraintPriority,true);
      if (allDiffApproxEquivMode || tspApproxEquivMode || precedenceApproxEquivMode)
         mdd->getSpec().useApproximateEquivalence();
      mdd->getSpec().setNodePriorityAggregateStrategy(nodePriorityAggregateStrategy);
      mdd->getSpec().setCandidatePriorityAggregateStrategy(candidatePriorityAggregateStrategy);
      MDDPBitSequence::Ptr all;
      MDDPBitSequence::Ptr allup;
      mdd->post(Factory::allDiffMDD2(vars,all,allup,allDiffOpts));
      mdd->post(Factory::tspSumMDD(vars,matrix,all,allup,z,obj,tspOpts));

//Since first and last variable values are set, we don't add precedence constraints for them
      int i = 0;
      for (auto row : matrix) {
         if (i == numVars - 1) break;
         int j = 0;
         for (auto cell : row) {
            if (j > 0 && cell < 0) {
// -1 means it can't go from i to j because of precedence.  So require j before i
               mdd->post(Factory::requiredPrecedenceMDD(vars,j,i,precedenceOpts));
            }
            j++;
         }
         i++;
      }

      cp->post(mdd);
   } else if (mode == 2) {
      cout << "MDD encoding with GOC" << endl;
      mdd = new MDDRelax(cp,width,maxRebootDistance,maxSplitIter,approxThenExact,maxConstraintPriority,true);
      if (allDiffApproxEquivMode || tspApproxEquivMode || precedenceApproxEquivMode)
         mdd->getSpec().useApproximateEquivalence();
      mdd->getSpec().setNodePriorityAggregateStrategy(nodePriorityAggregateStrategy);
      mdd->getSpec().setCandidatePriorityAggregateStrategy(candidatePriorityAggregateStrategy);
      MDDPBitSequence::Ptr all;
      MDDPBitSequence::Ptr allup;
      mdd->post(Factory::allDiffMDD2(vars,all,allup,allDiffOpts));
      mdd->post(Factory::tspSumMDD(vars,matrix,all,allup,z,obj,tspOpts));

      std::vector<std::pair<int,int>> precedences;
      int i = 0;
      for (auto row : matrix) {
         if (i == numVars - 1) break;
         int j = 0;
         for (auto cell : row) {
            if (j > 0 && cell < 0) {
// -1 means it can't go from i to j because of precedence.  So require j before i
               precedences.push_back(std::make_pair(j,i));
            }
            j++;
         }
         i++;
      }
      mdd->post(Factory::gocMDD2(vars,precedences,precedenceOpts));

      cp->post(mdd);
   } else {
      cout << "Error: specify a mode in {0,1}:" << endl;
      cout << "  0: classic" << endl;
      cout << "  1: MDD encoding" << endl;
      exit(1);
   }
   
   auto start = RuntimeMonitor::now();
   BFSearch search(cp,[=]() {
      auto x = selectFirst(vars,[](auto xk) { return xk->size() >1;});
      if (x) {
         //int c = x->min();
         //c = bestValue(mdd,x);
         //c = mdd->selectValueFor(x);
         //return  [=] {
         //   cp->post(x == c);
         //} | [=] {
         //   cp->post(x != c);
         //};
         std::vector<std::function<void(void)>> branches;
         for (int i = x->min(); i <= x->max(); i++) {
            if (x->contains(i)) branches.push_back([=] { cp->post(x == i); });
         }
         return Branches(branches);
      } else return Branches({});
   });
      
   SearchStatistics stat;
   search.onSolution([&stat,&vars,&obj,start,z]() {
       cout << "z->min() : " << z->min() << ", z->max() : " << z->max() << endl;
       cout << "obj : " << obj->value() << " " << vars << endl;
       cout << "#C  : " << stat.numberOfNodes() << "\n";
       cout << "#F  : " << stat.numberOfFailures() << endl;
       cout << "Time  : " << RuntimeMonitor::elapsedSince(start) << endl;
   });

   search.optimize(obj,stat,[obj](const SearchStatistics& stats) {
      return RuntimeMonitor::elapsedSeconds(stats.startTime()) > 3600;
   });
   auto dur = RuntimeMonitor::elapsedSince(start);
   std::cout << "Time : " << dur << '\n';
   cout << stat << endl;
   std::cout << "{ \"JSON\" :\n {";
   std::cout << "\n\t\"SOP\" :" << "{\n";
   std::cout << "\t\t\"primal\" : " << obj->primal() << ",\n";
   std::cout << "\t\t\"dual\" : " << obj->dual() << ",\n";
   std::cout << "\t\t\"optimalityGap\" : " << obj->optimalityGap() << ",\n";
   std::cout << "\t\t\"nodes\" : " << stat.numberOfNodes() << ",\n";
   std::cout << "\t\t\"fails\" : " << stat.numberOfFailures() << ",\n";
   std::cout << "\t\t\"time\" : " << dur << "\n";
   std::cout << "\t}\n";
   std::cout << "}\n}\n";
      
   //cp.dealloc();
}

int main(int argc,char* argv[])
{
   int fileIndex = (argc >= 2 && strncmp(argv[1],"-f",2)==0) ? atoi(argv[1]+2) : 1;
   int width = (argc >= 3 && strncmp(argv[2],"-w",2)==0) ? atoi(argv[2]+2) : 64;
   int mode  = (argc >= 4 && strncmp(argv[3],"-m",2)==0) ? atoi(argv[3]+2) : 1;
   int maxSplitIter = (argc >= 5 && strncmp(argv[4],"-i",2)==0) ? atoi(argv[4]+2) : 1;
   int maxRebootDistance = (argc >= 6 && strncmp(argv[5],"-r",2)==0) ? atoi(argv[5]+2) : 0;
   int nodePriorityMode = (argc >= 7 && strncmp(argv[6],"-n",2)==0) ? atoi(argv[6]+2) : 0;
   int nodePriorityAggregateStrategy = (argc >= 8 && strncmp(argv[7],"-na",3)==0) ? atoi(argv[7]+3) : 0;
   int candidatePriorityMode = (argc >= 9 && strncmp(argv[8],"-c",2)==0) ? atoi(argv[8]+2) : 0;
   int candidatePriorityAggregateStrategy = (argc >= 10 && strncmp(argv[9],"-ca",3)==0) ? atoi(argv[9]+3) : 0;
   int approxThenExact = (argc >= 11 && strncmp(argv[10],"-e",2)==0) ? atoi(argv[10]+2) : 0;
   int allDiffPriority  = (argc >= 12 && strncmp(argv[11],"-alldiffP",9)==0) ? atoi(argv[11]+9) : 0;
   int tspPriority  = (argc >= 13 && strncmp(argv[12],"-tspP",5)==0) ? atoi(argv[12]+5) : 0;
   int precedencePriority  = (argc >= 14 && strncmp(argv[13],"-precP",6)==0) ? atoi(argv[13]+6) : 0;
   int allDiffApproxEquivMode  = (argc >= 15 && strncmp(argv[14],"-alldiffEQ",10)==0) ? atoi(argv[14]+10) : 1;
   int tspApproxEquivMode  = (argc >= 16 && strncmp(argv[15],"-tspEQ",6)==0) ? atoi(argv[15]+6) : 1;
   int precedenceApproxEquivMode  = (argc >= 17 && strncmp(argv[16],"-precEQ",7)==0) ? atoi(argv[16]+7) : 1;

   vector<string> matrixFiles = {"esc07.sop",     //0
                                "esc11.sop",     //1
                                "esc12.sop",     //2
                                "esc25.sop",     //3
                                "esc47.sop",     //4
                                "esc63.sop",     //5
                                "esc78.sop",     //6
                                "br17.10.sop",   //7
                                "br17.12.sop",   //8
                                "ft53.1.sop",    //9
                                "ft53.2.sop",    //10
                                "ft53.3.sop",    //11
                                "ft53.4.sop",    //12
                                "ft70.1.sop",    //13
                                "ft70.2.sop",    //14
                                "ft70.3.sop",    //15
                                "ft70.4.sop",    //16
                                "kro124p.1.sop", //17
                                "kro124p.2.sop", //18
                                "kro124p.3.sop", //19
                                "kro124p.4.sop", //20
                                "p43.1.sop",     //21
                                "p43.2.sop",     //22
                                "p43.3.sop",     //23
                                "p43.4.sop",     //24
                                "prob.42.sop",   //25
				"prob.100.sop",  //26
                                "rbg048a.sop",   //27
                                "rbg050c.sop",   //28
                                "rbg109a.sop",   //29
                                "rbg150a.sop",   //30
                                "rbg174a.sop",   //31
                                "rbg253a.sop",   //32
                                "rbg323a.sop",   //33
                                "rbg341a.sop",   //34
                                "rbg358a.sop",   //35
                                "rbg378a.sop",   //36
                                "ry48p.1.sop",   //37
                                "ry48p.2.sop",   //38
                                "ry48p.3.sop",   //39
                                "ry48p.4.sop"};  //40
   const string matrixFile = "/Users/rebeccagentzel/minicpp/data/" + matrixFiles[fileIndex];

   cout << "width = " << width << endl;   
   cout << "mode = " << mode << endl;
   cout << "allDiffPriority = " << allDiffPriority << endl;
   cout << "tspPriority = " << tspPriority << endl;
   cout << "precedencePriority = " << precedencePriority << endl;
   cout << "allDiffApproxEquivMode = " << allDiffApproxEquivMode << endl;
   cout << "tspApproxEquivMode = " << tspApproxEquivMode << endl;
   cout << "precedenceApproxEquivMode = " << precedenceApproxEquivMode << endl;

   try {
      vector<string> content(readData(matrixFile.c_str()));
      auto it = content.begin();
      while (*(it++) != "EDGE_WEIGHT_SECTION");
      int numVars = std::stoi(*it);
      vector<vector<int> > matrix;
      while (*(++it) != "EOF")
         matrix.push_back(splitAsInt(*it, ' ', true));
      buildModel(numVars, matrix, mode, width, maxSplitIter, maxRebootDistance, nodePriorityMode, nodePriorityAggregateStrategy, candidatePriorityMode, candidatePriorityAggregateStrategy, approxThenExact, allDiffPriority, tspPriority, precedencePriority, allDiffApproxEquivMode, tspApproxEquivMode, precedenceApproxEquivMode);
   } catch (std::exception& e) {
      std::cerr << "Unable to find the file" << '\n';
   }
   
   return 0;
}
