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
// #include "mddrelax.hpp"
// #include "mddConstraints.hpp"

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
   Instance(int nc,int no,int ncof,vector<int>& lb,vector<int>& ub,
            vector<int>& demand,vector<vector<int>>& require)
      : _nbCars(nc), _nbOpts(no), _nbConf(ncof), _lb(lb), _ub(ub), _demand(demand), _require(require) {}
   int nbCars() const { return _nbCars;}
   int nbOpts() const { return _nbOpts;}
   int nbConf() const { return _nbConf;}
   vector<std::set<int>> options()
   {
      vector<std::set<int>> opts;
      for(int o = 0; o < _nbOpts; o++){
         std::set<int> vx;
         for(int c = 0; c < _nbConf; c++)
               if(_require[c][o])
                  vx.insert(c);
         opts.push_back(vx);
      }
      return opts;
   }
   vector<int> cars()
   {
      vector<int> ca;
      for(int c = 0; c < _nbConf; c++)
         for(int d = 0; d < _demand[c]; d++)
            ca.push_back(c);
      return ca;
   }
   int lb(int i) const { return _lb[i];}
   int ub(int i) const { return _ub[i];}
   int demand(int i) const { return _demand[i];}
   int requires(int i,int j) const { return _require[i][j];}
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

vector<int> splitAsInt(const std::string& str,char d)
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
      vector<vector<int>> requires(nbConf);
      vector<int> demands(nbConf);
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

template <typename Fun> vector<int> toVec(int min,int max,Fun f)
{
   std::vector<int> v;
   for(int i=min;i <= max;i++)
      v.emplace_back(f(i));
   return v;
}

void solveModel(CPSolver::Ptr cp,const Veci& line,const Instance& in)
{
   auto start = RuntimeMonitor::now();
   DFSearch search(cp,[=]() {

       unsigned i = 0u;
       for(i=0u;i < line.size();i++)
	 if (line[i]->size()> 1) break;
       auto x = i< line.size() ? line[i] : nullptr;
       
       // auto x = selectMin(line,
       //                   [](const auto& x) { return x->size() > 1;},
       //                   [](const auto& x) { return x->size();});

      if (x) {
         int c = x->min();
         
         return  [=] {
                    cp->post(x == c);
                 }
            | [=] {
                 cp->post(x != c);
              };
      } else return Branches({});
   });

   search.onSolution([&line,&in]() {
                        cout << line << endl;
                        for(int o=0;o < in.nbOpts();o++) {
                           std::cout << in.lb(o) << '/' << in.ub(o) << ' ';
                           for(int c = 0;c < in.nbCars();c++) {
                              if (in.requires(line[c]->min(),o))
                                 std::cout << 'Y';
                              else std::cout << ' ';
                           }
                           std::cout << std::endl;
                        }                      
                     });

   auto stat = search.solve([](const SearchStatistics& stats) {
                               return stats.numberOfSolutions() > 0;
                            });
   auto dur = RuntimeMonitor::elapsedSince(start);
   std::cout << "Time : " << dur << std::endl;
   cout << stat << endl;
}

void addCumulSeq(CPSolver::Ptr cp, const Veci& vars, int N, int L, int U, const std::set<int> S) {

  int H = (int)vars.size();
  
  auto cumul = Factory::intVarArray(cp, H+1, 0, H); 
  cp->post(cumul[0] == 0);
    
  auto boolVar = Factory::boolVarArray(cp, H);
  for (int i=0; i<H; i++) {
    cp->post(isMember(boolVar[i], vars[i], S));
  }
    
  for (int i=0; i<H; i++) {
    cp->post(equal(cumul[i+1], cumul[i], boolVar[i]));
  }
    
  for (int i=0; i<H-N+1; i++) {
    cp->post(cumul[i+N] <= cumul[i] + U);
    cp->post(cumul[i+N] >= cumul[i] + L);
  }
  
}

void buildModel(CPSolver::Ptr cp, Instance& in)
{
   using namespace std;

   auto cars = in.cars();
   auto options = in.options();
   int mx = in.nbConf()-1;
   //int nbC = (int) cars.size();
   int nbO = (int) options.size();

   auto line = Factory::intVarArray(cp,(int) cars.size(), 0, mx);

   // meet demand: count occurrence of configuration via a Boolean variable
   for(int i=0; i<in.nbConf(); i++) {
     auto boolVar = Factory::boolVarArray(cp,(int) cars.size());
     std::set<int> S;
     S.insert(i);
     for (int i=0; i<(int) cars.size(); i++) {
       cp->post(isMember(boolVar[i], line[i], S));
     }
     cp->post(sum(boolVar) == in.demand(i));
   }
   

   // sequence constraints: use cumulative encoding
   for(int o = 0; o < nbO; o++){
     set<int> Confs;
     for(int i=0; i<in.nbConf(); i++) {
       if ( in.requires(i,o) ) { Confs.insert(i); }
     }
     addCumulSeq(cp, line, in.ub(o), 0, in.lb(o), Confs);
   }

   solveModel(cp,line,in);
}


int main(int argc,char* argv[])
{
   const char* filename = (argc >= 2) ? argv[1] : "data/dataMini";

   std::cout << "filename = " << filename << std::endl;   

   try {
      Instance in = Instance::readData(filename);
      std::cout << in << std::endl;
      CPSolver::Ptr cp  = Factory::makeSolver();
      buildModel(cp,in);
   } catch (std::exception e) {
      std::cerr << "Unable to find the file" << std::endl;
   }

   return 0;
}
