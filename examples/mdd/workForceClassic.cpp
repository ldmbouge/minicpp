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

#include "solver.hpp"
#include "trailable.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"
#include "RuntimeMonitor.hpp"
#include "matrix.hpp"


using namespace std;
using namespace Factory;

class Job
{
private:
   int _start;
   int _end;
   int _duration;
public:
   Job(int start, int end, int duration) : _start(start), _end(end), _duration(duration) {}
   Job(vector<int> vec) : _start(vec[0]), _end(vec[1]), _duration(vec[2]) {}
   friend std::ostream &operator<<(std::ostream &output, const Job& j){
      return output << "{s:"<< j._start << ",e:" << j._end << ",d:" << j._duration << "}";
   }
   inline int start() {return _start;}
   inline int end() {return _end;}
   inline int duration() {return _duration;}
   inline bool overlap(const Job& j) {return max(_start,j._start) < min(_end,j._end);}
};

bool is_number(const std::string& s)
{
    return( strspn( s.c_str(), "-.0123456789" ) == s.size() );
}

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


vector<vector<int>> csv(const char * filename, bool header)
{
   vector<string> content(readData(filename));
   vector<vector<int>> res;
   if(header) content.erase(content.begin());
   for (auto& l : content){
      res.push_back(splitAsInt(l, ',', true));
   }
   return res;
}

vector<Job> makeJobs(vector<vector<int>> js)
{
   vector<Job> jobs;
   for(auto& j : js)
      jobs.push_back(Job(j));
   return jobs;
}
set<set<int>> sweep(vector<Job>& jobs)
{
   set<set<int>> cliques;
   using Evt = tuple<int,bool,int>;
   vector<Evt> pt;
   for(auto i = 0u; i < jobs.size(); i++){
      pt.push_back(make_tuple(jobs[i].start(),true,i));
      pt.push_back(make_tuple(jobs[i].end(),false,i));
   }
   sort(pt.begin(),pt.end(),[](const auto& e0,const auto& e1) {
                               return (std::get<0>(e0) < std::get<0>(e1)) 
                                  || (std::get<0>(e0) == std::get<0>(e1) && std::get<1>(e0) > std::get<1>(e1));
                            });
   for(const auto& e : pt) {
      bool isStart = std::get<1>(e);
      std::cout << ((isStart) ? '+' : '-') << " " << std::get<2>(e) << " || ";
      std::cout << std::get<0>(e) << " : " << jobs[std::get<2>(e)] << std::endl;
   }
   set<int> clique;
   bool added = false;
   for(const auto& p: pt){
      if(get<1>(p)){
         clique.insert(get<2>(p));
      }else{
         if(added)
            cliques.insert(clique);
         clique.erase(get<2>(p));
      }
      added = get<1>(p);
   }
   return cliques;
}

Veci all(CPSolver::Ptr cp,const set<int>& over, std::function<var<int>::Ptr(int)> clo)
{
   auto res = Factory::intVarArray(cp, (int) over.size());
   int i = 0;
   for(auto e : over){
      res[i++] = clo(e);
   }
   return res;
}

std::ostream& operator<<(std::ostream& os,const set<int>& s)
{
   os << '{';
   for(auto i : s)
      os << i << ',';
   return os << "\b}";
}

void checkSolution(Objective::Ptr obj,Factory::Veci& emp,set<set<int>>& cliques,vector<vector<int>>& compat)
{
   size_t nbJ = emp.size();
   int score = 0;
   for(unsigned j=0;j < nbJ;j++)
      score += compat[j][emp[j]->min()];
   std::cout  << "CHECK:" << score << " " << obj->value() << std::endl;
   unsigned  allOk = 0;
   for(auto& c : cliques) {
      unsigned nbEq = 0;
      for(auto i : c)
         for(auto j : c) 
            nbEq += emp[j]->min() == emp[i]->min();
      //std::cout << "CL: " << c << " EQ = " << nbEq << " CLSize:" << c.size() << std::endl;
      allOk += nbEq == c.size();
   }
   if (allOk != cliques.size())
      std::cout << "BAD Solution" << std::endl;
   else std::cout << "ALL good" << std::endl;
}

std::string tab(int d) {
   std::string s = "";
   while (d--!=0)
      s = s + "  ";
   return s;
}

void buildModel(CPSolver::Ptr cp, vector<Job>& jobs, vector<vector<int>> compat)
{
   using namespace std;
   int nbE = (int) compat.size();
   set<set<int>> cliques = sweep(jobs);
   auto emp = Factory::intVarArray(cp,(int) jobs.size(), 0, nbE-1);
   
   for(auto& c : cliques) {
      std::cout << "Clique: " << c << std::endl;
      auto adv = all(cp, c, [&emp](int i) {return emp[i];});
      cp->post(Factory::allDifferent(adv));
   }

   
   auto sm = Factory::intVarArray(cp,nbE,[&](int i) { return Factory::element(compat[i],emp[i]);});
   Objective::Ptr obj = Factory::maximize(Factory::sum(sm));
  
   auto start = RuntimeMonitor::now();
   DFSearch search(cp,[=]() {
                         /*                         auto x = selectMin(emp,
                                            [](const auto& x) { return x->size() > 1;},
                                            [](const auto& x) { return x->size();});
                         */
                         int depth = 0;
                         for(int i=0;i < nbE;i++) 
                            depth += emp[i]->size() == 1;
                                                                        
      unsigned i;      
      // for(i=0u;i< emp.size();i++)
      //    cout << emp[i] << endl;
      for(i=0u;i< emp.size();i++) 
         if (emp[i]->size() > 1)
            break;     
      auto x = i < emp.size() ? emp[i] : nullptr;                                                
      // cout << "picked: " << i << endl;
                         
                         
      if (x) {
         int i = x->getId();
         int largest = std::numeric_limits<int>::min();
         int bv = -1;
         for(auto v=0u;v < compat[i].size();v++) {
            if (!emp[i]->contains(v))
               continue;
            bv = compat[i][v] > largest ? v : bv;
            largest = std::max(largest,compat[i][v]);           
         }
         return  [=] {
                    // cout << tab(depth) << "?x(" << i << ") == " << bv << " " <<  x << endl;
                    cp->post(x == bv);
                    // cout << tab(depth) << "!x(" << i << ") == " << bv << endl;
                 }
            | [=] {
                 // cout << tab(depth) << "?x(" << i << ") != " << bv << " FAIL" << endl;
                 cp->post(x != bv);
                 // cout << tab(depth) << "!x(" << i << ") != " << bv << endl;
              };
      } else return Branches({});
   });

   SearchStatistics stat;
   search.onSolution([&emp,obj,&stat]() {
                        cout << "obj : " << obj->value() << " " << emp << endl;
                        cout << "#F  : " << stat.numberOfFailures() << endl;
                     });   
   search.optimize(obj,stat);   
   auto dur = RuntimeMonitor::elapsedSince(start);
   std::cout << "Time : " << dur << std::endl;
   cout << stat << endl;
}

int main(int argc,char* argv[])
{
   const char* jobsFile = "data/workforce9-jobs.csv";
   const char* compatFile = "data/workforce9.csv";
   try {
      auto jobsCSV = csv(jobsFile,true);
      auto compat = csv(compatFile,false);
      auto jobs = makeJobs(jobsCSV);
      for (auto& j : jobs)
         cout << j << std::endl;
      CPSolver::Ptr cp  = Factory::makeSolver();
      buildModel(cp,jobs,compat);
   } catch (std::exception& e) {
      std::cerr << "Unable to find the file" << std::endl;
   }

   return 0;
}
