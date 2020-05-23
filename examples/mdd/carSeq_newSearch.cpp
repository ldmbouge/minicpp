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
#include <map>
#include "solver.hpp"
#include "trailable.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"
// #include "mddrelax.hpp"
// #include "mddConstraints.hpp"

#include "RuntimeMonitor.hpp"
#include "matrix.hpp"
#include <math.h>
#include <limits.h>

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

void solveModel(CPSolver::Ptr cp,const Veci& line, Instance& in, int timelimit)
{
  
  int nbVars = line.size();
  int nbO = (int) in.options().size();
  
  vector<int> nbCarsByOption(nbO, 0);
  for(int o=0; o<nbO; o++){
    for(int i=0; i<in.nbConf(); i++) {
      if ( in.requires(i,o) ) { nbCarsByOption[o] += in.demand(i); }
    }
  }
  
  vector< pair<int,int> > slack(nbO);
  cout << "slack = [";
  for (int opt=0; opt<nbO; opt++) {
    const int N = nbVars, P = in.lb(opt), Q = in.ub(opt), K = nbCarsByOption[opt];
    slack[opt].first = N - (K*Q)/P;  // from Pascal's caro.co model (he has N=100)
    // slack[opt].first = N - Q*(K/P) - (K%P);   // best bound (from Gen-sequence paper)
    // slack[opt].first = N - Q*(K/P);        // correct but weaker bound from Regin-Puget
    slack[opt].second = opt;
    cout << slack[opt].first << ' ';
  }
  cout << ']' << endl;
  std::sort(slack.begin(), slack.end());
  
  vector<int> optionOrder(nbO, 0);
  for (int i=0; i<nbO; i++){
    optionOrder[i]=slack[i].second;
  }
  cout << "optionOrder = " << optionOrder << endl;

  int mx = in.nbConf()-1;
  auto oline = Factory::intVarArray(cp,(int) line.size(), 0, mx);

  int mid = nbVars/2;
  int idx = 0;
  oline[idx] = line[mid];
  // cout << "oline[" << idx << "] = line[" << mid << "]" << endl;
  for (int i=1; i<=nbVars/2; i++){
    idx++;
    oline[idx] = line[mid-i];
    // cout << "oline[" << idx << "] = line[" << mid-i << "]" << endl;
    idx++;
    if (idx < nbVars) {
      oline[idx] = line[mid+i];
      // cout << "oline[" << idx << "] = line[" << mid+i << "]" << endl;
    }
  }    

  // set of car configurations per order
  vector< set<int> > confsForOption(nbO);
  for(int o=0; o<nbO; o++){
    for(int i=0; i<in.nbConf(); i++) {
      if (in.requires(i,o)) { confsForOption[o].insert(i); }
    }
  }
  for(int o=0; o<nbO; o++){
    cout << "configurations for option " << o << ":";
    for (std::set<int>::iterator it=confsForOption[o].begin(); it!=confsForOption[o].end(); ++it)
      cout << " " << *it;
    cout << endl;
  }

  // some computations for ssu value ordering heuristic
  // taken from Comet model carg:
  //   int sutil[o in Options] = 100*(sod[o] * ub[o]) / (nbCars * lb[o]);
  //   int ssu[c in Configs] = sum(o in Options: requires[o,c] == 1) sutil[o];
  vector<int> sutil(nbO);
  for(int o=0; o<nbO; o++){
    sutil[o] = 100*nbCarsByOption[o]*in.ub(o) / (nbVars*in.lb(o));
  }
  vector<int> ssu(in.nbConf());
  for(int c=0; c<in.nbConf(); c++) {
    ssu[c] = 0;
    for(int o=0; o<nbO; o++) {
      ssu[c] += (in.requires(c,o))*sutil[o];
    }
  }

 
  auto start = RuntimeMonitor::now();
  
   DFSearch search(cp,[=]() {

       /***
       int varIdx = -1; // selected variable index
       int optIdx = -1; // selected option index
       int valIdx = -1; // selected option index
       for (int o=0; o<nbO; o++) {
	 // cout << "consider option " << slack[o].second << endl;
	 for(auto i=0u;i < oline.size();i++) {
	   // cout << "consider online[" << i << "] = " << oline[i] <<  endl;
	   if (oline[i]->size()>1) {
	     int opt = slack[o].second;
	     bool in = false;  // variable has conf in this option set
	     bool out = false; // variable has conf out this option set
	     for (int d=oline[i]->min(); d<=oline[i]->max(); d++) {
	       if (oline[i]->contains(d)) {
		 if (confsForOption[opt].find(d) != confsForOption[opt].end()) {
		   in = true;
		   valIdx = d;
		   //cout << d << " is in the set" << endl;
		 }
		 else {
		   out = true;
		   //cout << d << " is not in the set" << endl;
		 }
	       }
	       if (in && out) {
		 varIdx = i;
		 optIdx = opt;

		 // I can probably add the "return" branching function here?
		 // cout << "selected oline[" << varIdx << "]=" << oline[varIdx]
		 //      << " and contains confs with and without option " << optIdx << endl;
		 
		 break;
	       }
	     }
	   }
	   if (varIdx>=0) {  // this is ugly, but it's just a quick test
	     break;
	   }
	 }
	 if (varIdx>=0) {
	   break;
	 }
       }
       
       auto x = varIdx>=0 ? oline[varIdx] : nullptr;
       ***/
       
       // Lexicographic ordering on oline, puts variable selection before option selection
       unsigned i = 0u;
       for(i=0u;i < oline.size();i++)
	 if (oline[i]->size()> 1) break;
       auto x = i< oline.size() ? oline[i] : nullptr;
 
       if (x) {	 
	 // cout << "select variable oline[" << varIdx << "] = " << x << endl;
	 // cout << "select variable oline[" << i << "] = " << x << endl;
	 int c = -1;

	 // select value by ssu heuristic
	 int minSSU = INT_MAX;
	 for (int d=x->min(); d<=x->max(); d++) {
	   if (x->contains(d) && ssu[d] < minSSU) {
	     minSSU = ssu[d];
	     c = d;
	   }
	 }

	 // // select value by option (per slack ordering)
	 // for (int o=0; o<nbO; o++) {
	 //   // cout << "consider option " << slack[o].second << endl;
	 //   int opt = slack[o].second; 
	 //   for (int d=x->min(); d<=x->max(); d++) {
	 //     if (x->contains(d)) {
	 //       if (confsForOption[opt].find(d) != confsForOption[opt].end()) {
	 // 	 cout << "select value " << d << endl;
	 // 	 c = d;
	 // 	 break;
	 //       }
	 //     }
	 //     if (c>=0) break;
	 //   }
	 //   if (c>=0) break;
	 // }
	 
         return  [=] {
	   // if (!lex) {
	   //   // std::cout << "oline[" << varIdx << "] in confsForOption[" << optIdx << "]" << std::endl;
	   //   cp->post(inside(x,confsForOption[optIdx]));
	   //   // cout << oline << endl;
	   // }
	   // else  {
	     // cout << "oline[" << i << "] == " << c << std::endl;
	     cp->post(x==c);
	     // cout << oline << endl;
	   // }
	 }
	 | [=] {
	   // if (!lex) {
	   //   // std::cout << "oline[" << varIdx << "] notin confsForOption[" << optIdx << "]" << std::endl;
	   //   cp->post(outside(x,confsForOption[optIdx]));
	   //   // cout << oline << endl;
	   // }
	   // else {
	     // cout << "oline[" << i << "] != " << c << std::endl;
	     cp->post(x!=c);
	     // cout << oline << endl;
	   // }
	 };
       }
       else {
	 return Branches({});
       }
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

   auto stat = search.solve([timelimit](const SearchStatistics& stats) {
       return ((stats.numberOfSolutions() > 10000) || (RuntimeMonitor::elapsedSince(stats.startTime()) > 1000*timelimit));
     });
   
   auto dur = RuntimeMonitor::elapsedSince(start);
   std::cout << "Time : " << dur << std::endl;
   cout << stat << endl;
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

  // for additional propagation: add Among constraints (sum over the boolVar)
  for (int i=0; i<H-N+1; i++) {
    set<int> amongVars;
    for (int j=i; j<i+N; j++)
      amongVars.insert(j);    
    auto adv = all(cp, amongVars, boolVar);
    cp->post(sum(adv) >= L);
    cp->post(sum(adv) <= U);
  }  
}

void buildModel(CPSolver::Ptr cp, Instance& in, int timelimit)
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
     for (int j=0; j<(int) cars.size(); j++) {
       cp->post(isMember(boolVar[j], line[j], S));
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

   solveModel(cp,line,in, timelimit);
}


int main(int argc,char* argv[])
{
   const char* filename = (argc >= 2) ? argv[1] : "data/dataMini";
   int timelimit = (argc >= 3 && strncmp(argv[2],"-t",2)==0) ? atoi(argv[2]+2) : 60;
   
   std::cout << "filename = " << filename << std::endl;   
   std::cout << "time limit = " << timelimit << std::endl;   

   try {
      Instance in = Instance::readData(filename);
      std::cout << in << std::endl;
      CPSolver::Ptr cp  = Factory::makeSolver();
      buildModel(cp,in,timelimit);
   } catch (std::exception e) {
      std::cerr << "Unable to find the file" << std::endl;
   }

   return 0;
}
