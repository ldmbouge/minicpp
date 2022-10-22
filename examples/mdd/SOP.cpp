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

void buildModel(int numVars, vector<vector<int>> matrix, int mode, int width) {
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
      cout << "MDD encoding" << endl;
      mdd = new MDDRelax(cp,width);
      mdd->getSpec().setCandidatePriorityAggregateStrategy(1);
      MDDPBitSequence::Ptr all;
      MDDPBitSequence::Ptr allup;
      mdd->post(Factory::allDiffMDD2(vars,all,allup));
      mdd->post(Factory::tspSumMDD(vars,matrix,all,allup,z,obj));

      int i = 0;
      for (auto row : matrix) {
         int j = 0;
         for (auto cell : row) {
            if (cell < 0) {
// -1 means it can't go from i to j because of precedence.  So require j before i
               mdd->post(Factory::requiredPrecedenceMDD(vars,j,i));
            }
            j++;
         }
         i++;
      }

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

   search.optimize(obj,stat,[](const SearchStatistics& stats) {
      return RuntimeMonitor::elapsedSeconds(stats.startTime()) > 3600;
   });
   auto dur = RuntimeMonitor::elapsedSince(start);
   std::cout << "Time : " << dur << '\n';
   cout << stat << endl;
   std::cout << "\t\t\"primal\" : " << obj->primal() << "\n";
   std::cout << "\t\t\"dual\" : " << obj->dual() << "\n";
   std::cout << "\t\t\"optimalityGap\" : " << obj->optimalityGap() << "\n";
      
   cp.dealloc();
}

int main(int argc,char* argv[])
{
   const char* matrixFile = (argc >= 2) ? argv[1] : "../data/esc12.sop";
   int width = (argc >= 3 && strncmp(argv[2],"-w",2)==0) ? atoi(argv[2]+2) : 64;
   int mode  = (argc >= 4 && strncmp(argv[3],"-m",2)==0) ? atoi(argv[3]+2) : 1;

   cout << "width = " << width << endl;   
   cout << "mode = " << mode << endl;

   try {
      vector<string> content(readData(matrixFile));
      auto it = content.begin();
      while (*(it++) != "EDGE_WEIGHT_SECTION");
      int numVars = std::stoi(*it);
      vector<vector<int> > matrix;
      while (*(++it) != "EOF")
         matrix.push_back(splitAsInt(*it, ' ', true));
      buildModel(numVars, matrix, mode, width);
   } catch (std::exception& e) {
      std::cerr << "Unable to find the file" << '\n';
   }
   
   return 0;
}
