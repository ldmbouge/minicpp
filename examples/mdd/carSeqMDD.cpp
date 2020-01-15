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
#include "solver.hpp"
#include "trailable.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"
#include "mdd.hpp"
#include "RuntimeMonitor.hpp"
#include "matrix.hpp"


using namespace std;
using namespace Factory;

class Instance
{
   private :
   int _nbCars;
   int _nbOpts;
   int _nbConf;
   std::vector<int> _lb;
   std::vector<int> _ub;
   std::vector<int> _demand;
   std::vector<std::vector<int>> _require;
   
public:
   Instance(int nc, int no, int ncof, std::vector<int>& lb, std::vector<int>& ub,  std::vector<int>& demand,  std::vector<std::vector<int>>& require) : _nbCars(nc), _nbOpts(no), _nbConf(ncof), _lb(lb), _ub(ub), _demand(demand), _require(require) {}
   inline int nbCars() const { return _nbCars;}
   inline int nbOpts() const { return _nbOpts;}
   inline int nbConf() const { return _nbConf;}
   std::vector<std::set<int>> options()
   {
      std::vector<std::set<int>> opts;
      for(int o = 0; o < _nbOpts; o++){
         std::set<int> vx;
         for(int c = 0; c < _nbConf; c++)
               if(_require[c][o])
                  vx.insert(c);
         opts.push_back(vx);
      }
      return opts;
   }
   std::vector<int> cars()
   {
      std::vector<int> ca;
      for(int c = 0; c < _nbConf; c++)
         for(int d = 0; d < _demand[c]; d++)
            ca.push_back(c);
      return ca;
   }
   int lb(int i) {return _lb[i];}
   int ub(int i) {return _ub[i];}
   int demand(int i) {return _demand[i];}
   int requires(int i,int j) {return _require[i][j];}
   friend std::ostream &operator<<(std::ostream &output, const Instance& i)
   {
      output << "{" << std::endl;
      output << "#cars:" <<  i._nbCars << " #conf:" <<  i._nbConf;
      output << " #opt:" <<  i._nbOpts  << std::endl;
      output << "lb: " <<  i._lb  << std::endl;
      output << "ub: " <<  i._ub  << std::endl;
      output << "demand: " <<  i._demand  << std::endl;
      output << "require: " <<  i._require  << std::endl;
      output << "}" << std::endl;
      return output;
   }
   static Instance readData(const char* filename);
};
      
void cleanRows(std::string& buf)
{
   buf = std::regex_replace(buf,std::regex("r"),"\n");
   buf = std::regex_replace(buf,std::regex("\n\n"),"\n");
}

template<typename T>
std::vector<T> split(const std::string& str,char d, std::function<T(const std::string&)> clo)
{
   auto result = std::vector<T>{};
   auto ss = std::stringstream{str};
   for (std::string line; std::getline(ss, line, d);)
      if(!line.empty())
         result.push_back(clo(line));

   return result;
}

std::vector<std::string> split(const std::string& str,char d)
{
   return split<std::string>(str,d,[] (const auto& line) -> std::string {return line;});
}

std::vector<int> splitAsInt(const std::string& str,char d)
{
   return split<int>(str,d,[] (const auto& s) -> int {return std::stoi(s);});
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
      auto stats = splitAsInt(lines[0],' ');
      int nbCars = stats[0];
      int nbOpts = stats[1];
      int nbConf = stats[2];
      auto lb = splitAsInt(lines[1],' ');
      auto ub = splitAsInt(lines[2],' ');
      std::vector<std::vector<int>> requires(nbConf);
      std::vector<int> demands(nbConf);
      for(int cid = 0; cid < nbConf;  cid++)
         requires[cid].resize(nbOpts);
      for(int cid = 0; cid < nbConf;  cid++){
         auto row = splitAsInt(lines[3+cid], ' ');
         demands[row[0]] = row[1];
         for(int o = 0; o < nbOpts;  o++)
            requires[row[0]][o] = row[2+o];
      }
      return Instance(nbCars,nbOpts,nbConf,lb,ub,demands,requires);
   }else
      throw;
}
std::map<int,int> tomap(int min, int max,std::function<int(int)> clo)
{
   std::map<int,int> r;
   for(int i = min; i <= max; i++)
      r[i] = clo(i);
   return r;
}

void solveModel(CPSolver::Ptr cp)
{
   auto vx = cp->intVars();
   DFSearch search(cp,[=]() {
      auto x = selectMin(vx,
                         [](const auto& x) { return x->size() > 1;},
                         [](const auto& x) { return x->size();});
      if (x) {
         int c = x->min();
         
         return  [=] {
            cp->post(x == c);}
         | [=] {
            cp->post(x != c);};
      } else return Branches({});
   });

   search.onSolution([&vx]() {
      std::cout << "Assignment:" << std::endl;
      std::cout << vx << std::endl;
   });

   auto stat = search.solve([](const SearchStatistics& stats) {
                               return stats.numberOfSolutions() > 0;
                            });
   cout << stat << endl;
}
void buildModel(CPSolver::Ptr cp, Instance& in)
{
   auto cars = in.cars();
   auto options = in.options();
   int mx = in.nbConf()-1;
   auto vars = Factory::intVarArray(cp,(int) cars.size(), 0, mx);
   int nbC = (int) cars.size();int nbO = (int) options.size();
   matrix<Veci, var<int>::Ptr, 2> setup(cp->getStore(),{nbC, nbO});
   auto mdd = new MDD(cp);
   gccMDD(mdd->getSpec(), vars, tomap(0, mx,[&in] (int i) -> int {return in.demand(i);}));
   std::cout << mdd->getSpec() << std::endl;
   for(int o = 0; o < nbO; o++){
      auto opts = Factory::intVarArray(cp, nbC);
      for(int c = 0; c < nbC; c++){
         setup[c][o] = makeIntVar(cp, 0, 1);
         opts[c] = setup[c][o];
      }
//      amongMDD(mdd.getSpec(),opts, in.lb(o), in.ub(o), {1});
//      if(in.ub(o) > 0)
      seqMDD(mdd->getSpec(),opts, in.ub(o), 0, in.lb(o), {1});
   }
   cp->post(mdd);
   solveModel(cp);
}


int main(int argc,char* argv[])
{
   const char* filename = "/Users/zitoun/Desktop/datao";
   try {
      Instance in = Instance::readData(filename);
      std::cout << in << std::endl;
      CPSolver::Ptr cp  = Factory::makeSolver();
      buildModel(cp,in);
//      solveModel(cp);
   } catch (std::exception e) {
      std::cerr << "Unable to find the file" << std::endl;
   }

   return 0;
}
