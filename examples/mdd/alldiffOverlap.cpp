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
#include "mddrelax.hpp"
#include "RuntimeMonitor.hpp"
#include "matrix.hpp"


using namespace std;
using namespace Factory;


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


std::string tab(int d) {
   std::string s = "";
   while (d--!=0)
      s = s + "  ";
   return s;
}

void buildModel(CPSolver::Ptr cp, int relaxSize, int useMDD)
{
   using namespace std;

   // Settings as in Andersen et al. [CP 2007]:
   // Three overlapping Alldiffs with parameters (n,r,d) where
   //   n = number of variables
   //   r = size of scope of each constraint
   //   d = domain size
   // The paper reports (10,9,9) and (12,10,10).
      
   int N = 12;
   int D = 10;
   auto vars = Factory::intVarArray(cp, N, 0, D-1);

   set<int> C1 = {0,1,  3,4,5,6,7,8,  10,11};
   set<int> C2 = {0,1,2,3,  5,6,  8,9,10,11};
   set<int> C3 = {0,1,2,  4,5,6,7,8,9,10};
   
   auto mdd = new MDDRelax(cp,relaxSize);
   auto adv1 = all(cp, C1, [&vars](int i) {return vars[i];});
   Factory::allDiffMDD(mdd->getSpec(),adv1);
   cp->post(Factory::allDifferent(adv1));

   auto adv2 = all(cp, C2, [&vars](int i) {return vars[i];});
   Factory::allDiffMDD(mdd->getSpec(),adv2);
   cp->post(Factory::allDifferent(adv2));
   
   auto adv3 = all(cp, C3, [&vars](int i) {return vars[i];});
   Factory::allDiffMDD(mdd->getSpec(),adv3);
   cp->post(Factory::allDifferent(adv3));
  

   // All added to single MDD
   if (useMDD != 0) {
     cp->post(mdd);
   }

   DFSearch search(cp,[=]() {
      unsigned i;
      for(i=0u;i< vars.size();i++)
         if (vars[i]->size() > 1)
            break;
      auto x = i < vars.size() ? vars[i] : nullptr;
      /*
       auto x = selectMin(vars,
			  [](const auto& x) { return x->size() > 1;},
			  [](const auto& x) { return x->size();});
      */   
       if (x) {
	 int c = x->min();

         return  [=] {
                    //cout << tab(i) << "?x(" << i << ") == " << c << endl;
                    cp->post(x == c);
                    //cout << tab(i) << "!x(" << i << ") == " << c << endl;
                 }
            | [=] {
                 //cout << tab(i) << "?x(" << i << ") != " << c << " FAIL" << endl;
                 cp->post(x != c);
                 //cout << tab(i) << "!x(" << i << ") != " << c << endl;
              };
       } else return Branches({});
     });
      
   search.onSolution([&vars]() {
       std::cout << "Assignment:" << std::endl;
       std::cout << vars << std::endl;
     });
      
      
   auto stat = search.solve([](const SearchStatistics& stats) {
       return stats.numberOfSolutions() > 0;
     });
   cout << stat << endl;
}

int main(int argc,char* argv[])
{
   int width = (argc >= 2 && strncmp(argv[1],"-w",2)==0) ? atoi(argv[1]+2) : 2;
   int useMDD  = (argc >= 3 && strncmp(argv[2],"-m",2)==0) ? atoi(argv[2]+2) : 1;
 
   std::cout << "width = " << width << std::endl;
   std::cout << "useMDD = " << useMDD << std::endl;
   try {
      CPSolver::Ptr cp  = Factory::makeSolver();
      buildModel(cp, width, useMDD);
   } catch (std::exception& e) {
      std::cerr << "Unable to find the file" << std::endl;
   }

   return 0;
}
