
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
#include "solver.hpp"
#include "trailable.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"
#include "mddrelax.hpp"
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
   for(int i = 0; i < jobs.size(); i++){
      pt.push_back(make_tuple(jobs[i].start(),true,i));
      pt.push_back(make_tuple(jobs[i].end(),false,i));
   }
   sort(pt.begin(),pt.end(),[](const auto& e0,const auto& e1) {return (std::get<0>(e0) < std::get<0>(e1));});
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
void solveModel(CPSolver::Ptr cp,Veci& vars, const vector<Job>& jobs,Objective::Ptr obj)
{
//   auto vars = cp->intVars();
   auto start = RuntimeMonitor::now();
   DFSearch search(cp,[=]() {
      auto x = selectMin(vars,
                         [](const auto& x) { return x->size() > 1;},
                         [](const auto& x) { return x->size();});
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
   
   search.onSolution([&vars,obj]() {
      cout << "obj : " << obj->value() << " ";
                        cout << vars << endl;
                     });

   
   auto stat = search.optimize(obj);
   auto dur = RuntimeMonitor::elapsedSince(start);
   std::cout << "Time : " << dur << std::endl;
   cout << stat << endl;
}

void buildModel(CPSolver::Ptr cp, vector<Job>& jobs, vector<vector<int>> compat, int relaxSize)
{
   int nbE = (int) compat.size();
   auto cliques = sweep(jobs);
   auto emp = Factory::intVarArray(cp,(int) jobs.size(), 0, nbE-1);

   for(const set<int>& c : cliques){
      auto mdd = new MDDRelax(cp,relaxSize);
      Factory::allDiffMDD(mdd->getSpec(),all(cp, c, [&emp](int i) {return emp[i];}));
      cp->post(mdd);
   }
   auto sm = Factory::intVarArray(cp, nbE);
   for(int i = 0; i < nbE; i++){
      auto t = Factory::intVarArray(cp, (int) compat[i].size(), [i,&cp,&compat] (int j) {return Factory::makeIntVar(cp, compat[i][j], compat[i][j]);});
      sm[i] = t[emp[i]];
   }
   Objective::Ptr obj = Factory::minimize(Factory::sum(sm));
   solveModel(cp, emp, jobs, obj);
}

int main(int argc,char* argv[])
{
   const char* jobsFile = "/Users/zitoun/work/minicpp/minicpp/examples/data/workforce9-jobs.csv";
   const char* compatFile = "/Users/zitoun/work/minicpp/minicpp/examples/data/workforce9.csv";
   int relaxationSize = 32;
   try {
      auto jobsCSV = csv(jobsFile,true);
      auto compat = csv(compatFile,false);
      auto jobs = makeJobs(jobsCSV);
      for (auto& j : jobs)
         cout << j << std::endl;
      CPSolver::Ptr cp  = Factory::makeSolver();
      buildModel(cp,jobs,compat,relaxationSize);
   } catch (std::exception e) {
      std::cerr << "Unable to find the file" << std::endl;
   }

   return 0;
}
