#include <string>
#include <cstring>
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
#include "mddrelax.hpp"
#include "mddConstraints.hpp"
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
   int _numConflicts;
   int _numConflictsDynamic;
   int _jobID;
   int _largest;
   int _sum;
public:
   Job(int start, int end, int duration) : _start(start), _end(end), _duration(duration) { _numConflicts = _numConflictsDynamic = 0; }
   Job(vector<int> vec) : _start(vec[0]), _end(vec[1]), _duration(vec[2]) { _numConflicts = _numConflictsDynamic = 0; }
   void setID(int id) { _jobID = id; }
   int jobID() { return _jobID; }
   void setLargest(int largest) { _largest = largest; }
   void setSum(int sum) { _sum = sum; }
   void addConflict() { _numConflicts++; }
   int numConflicts() { return _numConflicts; }
   void addConflictDynamic() { _numConflictsDynamic++; }
   int numConflictsDynamic() { return _numConflictsDynamic; }
   static int SortMethod;
   friend std::ostream &operator<<(std::ostream &output, const Job& j){
      return output << "{s:"<< j._start << ",e:" << j._end << ",d:" << j._duration << "}";
   }
   inline int start() {return _start;}
   inline int end() {return _end;}
   inline int duration() {return _duration;}
   inline bool overlap(const Job& j) {return max(_start,j._start) < min(_end,j._end);}
   friend bool operator<(const Job& j1, const Job& j2);
};
int Job::SortMethod = 0;
bool operator<(const Job& j1, const Job& j2) { 
   switch (Job::SortMethod) {
      case 1:
         return (j1._start < j2._start) || (j1._start == j2._start && j1._end < j2._end);
      case 2:
         return (j1._end < j2._end) || (j1._end == j2._end && j1._start < j2._start);
      case 3:
         return (j1._numConflicts < j2._numConflicts) || (j1._numConflicts == j2._numConflicts && ((j1._start < j2._start) || (j1._start == j2._start && j1._end < j2._end)));
      case 4:
         return (j1._numConflicts > j2._numConflicts) || (j1._numConflicts == j2._numConflicts && ((j1._start < j2._start) || (j1._start == j2._start && j1._end < j2._end)));
      case 5:
         return (j1._largest > j2._largest) || (j1._largest == j2._largest && ((j1._start < j2._start) || (j1._start == j2._start && j1._end < j2._end)));
      case 6:
         return (j1._largest < j2._largest) || (j1._largest == j2._largest && ((j1._start < j2._start) || (j1._start == j2._start && j1._end < j2._end)));
      case 7:
         return (j1._sum > j2._sum) || (j1._sum == j2._sum && ((j1._start < j2._start) || (j1._start == j2._start && j1._end < j2._end)));
      case 8:
         return (j1._sum < j2._sum) || (j1._sum == j2._sum && ((j1._start < j2._start) || (j1._start == j2._start && j1._end < j2._end)));
   }
   return j1._jobID < j2._jobID;
}

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
      std::cout << std::get<0>(e) << " : " << jobs[std::get<2>(e)] << '\n';
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
   std::cout  << "CHECK:" << score << " " << obj->value() << '\n';
   unsigned  allOk = 0;
   for(auto& c : cliques) {
      unsigned nbEq = 0;
      for(auto i : c)
         for(auto j : c) 
            nbEq += emp[j]->min() == emp[i]->min();
      //std::cout << "CL: " << c << " EQ = " << nbEq << " CLSize:" << c.size() << '\n';
      allOk += nbEq == c.size();
   }
   if (allOk != cliques.size())
      std::cout << "BAD Solution" << '\n';
   else std::cout << "ALL good" << '\n';
}

std::string tab(int d) {
   std::string s = "";
   while (d--!=0)
      s = s + "  ";
   return s;
}

void buildModel(CPSolver::Ptr cp, vector<Job>& jobs, vector<vector<int>> compat, int mode, int relaxSize, int over, MDDOpts opts, int variableSearchHeuristic, int valueSearchHeuristic)
{
   using namespace std;
   auto numJobs = jobs.size();
   int nbE = (int) compat[0].size();

   for (auto i = 0u; i < numJobs; i++) {
      jobs[i].setID(i);
      int largest = compat[i][0];
      int sum = largest;
      for(int v=1;v < nbE;v++) {
         largest = std::max(largest,compat[i][v]);
         sum += compat[i][v];
      }
      jobs[i].setLargest(largest);
      jobs[i].setSum(sum);
      for (auto j = i+1; j < numJobs; j++) {
         if (jobs[i].end() >= jobs[j].start() && jobs[j].end() >= jobs[i].start()) {
            jobs[i].addConflict();
            jobs[j].addConflict();
         }
      }
   }

   if (Job::SortMethod == 9) {
      auto numSorted = 0u;
      while (numSorted < numJobs) {
         auto bestNextVar = numSorted;
         for (auto i = numSorted + 1; i < numJobs; i++) {
            if (jobs[i].numConflictsDynamic() > jobs[bestNextVar].numConflictsDynamic() || (jobs[i].numConflictsDynamic() == jobs[bestNextVar].numConflictsDynamic() && (jobs[i].numConflicts() > jobs[bestNextVar].numConflicts() || (jobs[i].numConflicts() == jobs[bestNextVar].numConflicts() && jobs[i].start() < jobs[bestNextVar].start())))) {
               bestNextVar = i;
            }
         }
         for (auto i = numSorted; i < numJobs; i++) {
            if (i == bestNextVar) continue;
            if (jobs[i].end() >= jobs[bestNextVar].start() && jobs[bestNextVar].end() >= jobs[i].start())
               jobs[i].addConflictDynamic();
         }
         iter_swap(jobs.begin() + numSorted, jobs.begin() + bestNextVar);
         numSorted++;
      }
   } else {
      std::sort(jobs.begin(),jobs.end());
   }

   vector<vector<int>> sortedCompat;
   for (auto i = 0u; i < numJobs; i++) {
      sortedCompat.push_back(compat[jobs[i].jobID()]);
   }

   //int nbJ = (int) compat.size();
   set<set<int>> cliques = sweep(jobs);
   vector<set<int>> cv;
   std::copy(cliques.begin(),cliques.end(),std::inserter(cv,cv.end()));
   auto emp = Factory::intVarArray(cp,(int) numJobs, 0, nbE-1);
   vector<bool> taken(cv.size()); // for each clique a boolean saying whether it was already picked up.
   vector<set<unsigned>> cid; // identifiers of cliques to bundle in the same MDD (identifiers refer to index within cv)

   for(auto i=0u;i < cv.size();i++) {
      if (taken[i]) continue;
      set<int> acc = cv[i];
      taken[i] = true;
      auto largest = acc.size();
      set<unsigned> chosen;
      chosen.insert(i);
      for(auto j = i+1;j < cv.size();j++) {
         if (taken[j]) continue;
         auto& c2 = cv[j];
         std::set<int> inter;
         std::set_intersection(acc.begin(),acc.end(),c2.begin(),c2.end(),std::inserter(inter,inter.begin()));
         largest = std::max(largest,c2.size());
         bool takeIt = inter.size() >= (over * largest)/100;
         if (takeIt) {
            acc = inter;
            taken[j] = true;
            chosen.insert(j);
         }
      }      
      cid.push_back(chosen);
      //ss += chosen.size();
   }
   
   // Test: add objective to each MDD
   int zUB = 0;
   for (unsigned int i=0; i<emp.size(); i++) {
     int tmpMax = 0;
     for (int j=emp[i]->min(); j<=emp[i]->max(); j++) {
        tmpMax = std::max(tmpMax,sortedCompat[i][j]);
     }
     zUB += tmpMax;
   }
   cout << "zUB = " << zUB << endl;
   auto z = Factory::makeIntVar(cp, 0, zUB);

   cout << "COMPAT MATRIX\n";
   for (unsigned int i=0; i<emp.size(); i++) {
      cout << "R" << setw(2) << i << ": ";
     for (int j=emp[i]->min(); j<=emp[i]->max(); j++) {
        std::cout << setw(2) << sortedCompat[i][j] << " ";
     }
     std::cout << "\n";
   }

   Objective::Ptr obj;

   //assert(ss == cv.size());
   cout << "CID.size():" << cid.size() << "\n";
   //int k = 0;
   MDDRelax::Ptr theOne = nullptr;
   if (mode == 0) {
      for(auto& c : cliques) {
         auto adv = all(cp, c, [&emp](int i) {return emp[i];});
         cp->post(Factory::allDifferent(adv));
      }
      auto sm = Factory::intVarArray(cp,numJobs,[&](int i) { return Factory::element(sortedCompat[i],emp[i]);});
      cp->post(z == sum(sm));
      obj = Factory::maximize(Factory::sum(sm));
   }
   if (mode == 1) {
      for(auto& c : cliques) {
         auto adv = all(cp, c, [&emp](int i) {return emp[i];});
         cp->post(Factory::allDifferentAC(adv));
      }
      auto sm = Factory::intVarArray(cp,numJobs,[&](int i) { return Factory::element(sortedCompat[i],emp[i]);});
      cp->post(z == sum(sm));
      obj = Factory::maximize(Factory::sum(sm));
   }
   if (mode == 2) {
      for(auto& ctm : cid) {
         //if (k++ < 10) continue;
         //auto mdd = new MDD(cp);
         auto mdd = Factory::makeMDDRelax(cp,relaxSize);
         for(auto theClique : ctm) {  // merge on cliques if normal alldiff.
            auto c = cv[theClique];
            std::cout << "Clique: " << c << '\n';
            auto adv = all(cp, c, [&emp](int i) {return emp[i];});
            mdd->post(Factory::allDiffMDD2(adv,opts));
            //cp->post(Factory::allDifferent(adv));
         }
         // add objective to MDD
         mdd->post(sum(emp, sortedCompat, z));
         cp->post(mdd);
         theOne = mdd;
         //mdd->saveGraph();
      }
      obj = Factory::maximize(z);
   }
   if (mode == 3) {
      for(auto& ctm : cid) {
         auto mdd = Factory::makeMDDRelax(cp,relaxSize);
         for(auto theClique : ctm) {  // merge on cliques if normal alldiff.
            auto c = cv[theClique];
            std::cout << "Clique: " << c << '\n';
            auto adv = all(cp, c, [&emp](int i) {return emp[i];});
            mdd->post(Factory::allDiffMDD2(adv,opts));
         }
         // add objective to MDD
         auto vars = mdd->getSpec().getVars();
         auto mddVars = Factory::intVarArray(cp, (int) vars.size());
         auto otherVars = Factory::intVarArray(cp, (int) numJobs - vars.size());
         vector<vector<int>> mddCompat;
         vector<vector<int>> otherCompat;
         int mddUB = 0;
         //int otherUB = 0;
         int mddVarCount = 0;
         int otherVarCount = 0;
         for(auto i = 0u; i < numJobs; i++) {
            int tmpMax = 0;
            for (int j=emp[i]->min(); j<=emp[i]->max(); j++) {
               tmpMax = std::max(tmpMax,sortedCompat[i][j]);
            }
            if(std::find(vars.cbegin(),vars.cend(),emp[i]) == vars.cend()) {
               otherVars[otherVarCount++] = emp[i];
               otherCompat.push_back(sortedCompat[i]);
               //otherUB += tmpMax;
            } else {
               mddVars[mddVarCount++] = emp[i];
               mddCompat.push_back(sortedCompat[i]);
               mddUB += tmpMax;
            }
         }
         auto mddVarsSum = Factory::makeIntVar(cp, 0, mddUB);
         mdd->post(sum(mddVars, mddCompat, mddVarsSum));
         auto sm = Factory::intVarArray(cp,otherVars.size(),[&](int i) { return Factory::element(otherCompat[i],otherVars[i]);});
         cp->post(z == mddVarsSum + sum(sm));
         cp->post(mdd);
         theOne = mdd;
      }
      auto sm = Factory::intVarArray(cp,numJobs,[&](int i) { return Factory::element(sortedCompat[i],emp[i]);});
      cp->post(z == sum(sm));
      obj = Factory::maximize(z);
   }

     // auto sm = Factory::intVarArray(cp,nbJ,[&](int i) { return Factory::element(sortedCompat[i],emp[i]);});
     // Objective::Ptr obj = Factory::maximize(Factory::sum(sm));

        // obj: 981
     	// #choice   : 5748
	// #fail     : 2875
	// #sols     : 5
	// completed : 0
     

   // Add separate MDD for objective
   // auto mddSum = new MDDRelax(cp,relaxSize);
   // Factory::sumMDD(mddSum->getSpec(), emp, compat, z);
   // cp->post(mddSum);

   //Objective::Ptr obj = Factory::maximize(z);
   
   auto start = RuntimeMonitor::now();
   DFSearch search(cp,[=]() {
      var<int>::Ptr x;
      switch (variableSearchHeuristic) {
         case 0:
            x = selectFirst(emp,[](const auto& x) { return x->size() > 1;});
            break;
         case 1:
            x = selectMin(emp,
                          [](const auto& x) { return x->size() > 1;},
                          [](const auto& x) { return x->size();});
            break;
      }

      if (x) {
         int bv;
         int i = x->getId();
         int largest;
         switch (valueSearchHeuristic) {
            case 0:
               largest = std::numeric_limits<int>::min();
               for(auto v=0u;v < sortedCompat[i].size();v++) {
                  if (!emp[i]->contains(v))
                     continue;
                  bv = sortedCompat[i][v] > largest ? v : bv;
                  largest = std::max(largest,sortedCompat[i][v]);     
               }
               break;
            case 1:
               bv = bestValue(theOne,x);
               break;
            case 2:
               bv = theOne->selectValueFor(x);
               break;
            case 3:
               bv = x->min();
               break;
         }
         //string tab(cp->getStateManager()->depth(),' ');
         return  [=] {
            //cout << tab << "?x(" << i << ") == " << bv << " -- " << x << endl;
            cp->post(x == bv);
            //cout << tab << "!x(" << i << ") == " << bv << endl;
         } | [=] {
            //cout << tab << "?x(" << i << ") != " << bv << " FAIL" << endl;
            cp->post(x != bv);
            //cout << tab << "!x(" << i << ") != " << bv << endl;
         };
      } else return Branches({});
   });

   SearchStatistics stat;
   search.onSolution([&emp,obj,z,&stat,theOne,start/*,&cliques,&sortedCompat*/]() {
       cout << "z->min() : " << z->min() << ", z->max() : " << z->max() << endl;
       cout << "obj : " << obj->value() << " " << emp << endl;
       cout << "#C  : " << stat.numberOfNodes() << "\n";
       cout << "#F  : " << stat.numberOfFailures() << endl;
       cout << "Time  : " << RuntimeMonitor::elapsedSince(start) << endl;
       //theOne->saveGraph();
       //exit(1);
       //checkSolution(obj,emp,cliques,sortedCompat);
   });   
   search.optimize(obj,stat);   
   auto dur = RuntimeMonitor::elapsedSince(start);
   std::cout << "Time : " << dur << '\n';
   cout << stat << endl;

   extern int iterMDD;
   extern int nbCSDown;
   extern int splitCS,pruneCS,potEXEC;
   extern int nbCONSCall,nbCONSFail;
   extern int nbAECall,nbAEFail;
   extern int hitCSDown;

   std::cout << "I/C  : " << (double)iterMDD/stat.numberOfNodes() << '\n';
   std::cout << "#CS  : " << nbCSDown << '\n';
   if (theOne) std::cout << "#L   : " << theOne->nbLayers() << '\n';

   extern int splitCS,pruneCS,potEXEC;
   std::cout << "SPLIT:" << splitCS << " \tpruneCS:" << pruneCS << " \tpotEXEC:" << potEXEC << '\n';

   std::cout << "{ \"JSON\" :\n {";
   std::cout << "\n\t\"workForce\" :" << "{\n";
   std::cout << "\t\t\"m\" : " << mode << ",\n";
   std::cout << "\t\t\"w\" : " << relaxSize << ",\n";
   std::cout << "\t\t\"nodes\" : " << stat.numberOfNodes() << ",\n";
   std::cout << "\t\t\"fails\" : " << stat.numberOfFailures() << ",\n";
   std::cout << "\t\t\"iter\" : " << iterMDD << ",\n";
   std::cout << "\t\t\"nbCSDown\" : " << nbCSDown << ",\n";
   if (theOne) std::cout << "\t\t\"layers\" : " << theOne->nbLayers() << ",\n";
   std::cout << "\t\t\"splitCS\" : " << splitCS << ",\n";
   std::cout << "\t\t\"pruneCS\" : " << pruneCS << ",\n";
   std::cout << "\t\t\"pot\" : " << potEXEC << ",\n";  
   std::cout << "\t\t\"hitCS\" : " << hitCSDown << ",\n";  
   std::cout << "\t\t\"time\" : " << dur << "\n";
   std::cout << "\t}\n";  
   std::cout << "}\n}";
}

int main(int argc,char* argv[])
{
   const char* jobsFile = "./data/workforce100-jobs.csv";
   const char* compatFile = "./data/workforce100.csv";
   int mode = (argc >= 2 && strncmp(argv[1],"-m",2)==0) ? atoi(argv[1]+2) : 1;
   int width = (argc >= 3 && strncmp(argv[2],"-w",2)==0) ? atoi(argv[2]+2) : 2;
   int over  = (argc >= 4 && strncmp(argv[3],"-o",2)==0) ? atoi(argv[3]+2) : 60;
   int nodePriority = (argc >= 5 && strncmp(argv[4],"-y",2)==0) ? atoi(argv[4]+2) : 0;
   int candidatePriority = (argc >= 6 && strncmp(argv[5],"-w",2)==0) ? atoi(argv[5]+2) : 0;
   int approxEquivMode = (argc >= 7 && strncmp(argv[6],"-e",2)==0) ? atoi(argv[6]+2) : 0;
   int equivThreshold = (argc >= 8 && strncmp(argv[7],"-t",2)==0) ? atoi(argv[7]+2) : 0;
   int varOrdering  = (argc >= 9 && strncmp(argv[8],"-v",2)==0) ? atoi(argv[8]+2) : -1;
   int variableSearchHeuristic  = (argc >= 10 && strncmp(argv[9],"-var",4)==0) ? atoi(argv[9]+4) : 0;
   int valueSearchHeuristic  = (argc >= 11 && strncmp(argv[10],"-val",4)==0) ? atoi(argv[10]+4) : 0;
   std::cout << "mode = " << mode << "\n";
   std::cout << "width = " << width << "\n";
   std::cout << "over = " << over << "\n";
   std::cout << "nodePriority = " << nodePriority << "\n";
   std::cout << "candidatePriority = " << candidatePriority << "\n";
   std::cout << "approxEquivMode = " << approxEquivMode << "\n";
   std::cout << "equivThreshold = " << equivThreshold << "\n";
   std::cout << "varOrdering = " << varOrdering << "\n";
   std::cout << "variableSearchHeuristic = " << variableSearchHeuristic << "\n";
   std::cout << "valueSearchHeuristic = " << valueSearchHeuristic << "\n";
   MDDOpts opts = {
      .nodeP = nodePriority,
      .candP = candidatePriority,
      .appxEQMode = approxEquivMode,
      .eqThreshold = equivThreshold
   };
   try {
      auto jobsCSV = csv(jobsFile,true);
      auto compat = csv(compatFile,false);
      auto jobs = makeJobs(jobsCSV);
      for (auto& j : jobs)
         cout << j << '\n';
      CPSolver::Ptr cp  = Factory::makeSolver();
      Job::SortMethod = varOrdering;
      buildModel(cp,jobs,compat,mode,width,over,opts,variableSearchHeuristic,valueSearchHeuristic);
   } catch (std::exception& e) {
      std::cerr << "Unable to find the file" << '\n';
   }

   return 0;
}
