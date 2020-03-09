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
   auto Vars = Factory::intVarArray(cp, N, 1, D);

   set<int> C1;
   C1.insert(0);
   C1.insert(1);
   //   C1.insert(2);
   C1.insert(3);
   C1.insert(4);
   C1.insert(5);
   C1.insert(6);
   C1.insert(7);
   C1.insert(8);
   //   C1.insert(9);
   C1.insert(10);
   C1.insert(11);
   set<int> C2;
   C2.insert(0);
   C2.insert(1);
   C2.insert(2);
   C2.insert(3);
   //   C2.insert(4);
   C2.insert(5);
   C2.insert(6);
   //   C2.insert(7);
   C2.insert(8);
   C2.insert(9);
   C2.insert(10);
   C2.insert(11);
   set<int> C3;
   C3.insert(0);
   C3.insert(1);
   C3.insert(2);
   //   C3.insert(3);
   C3.insert(4);
   C3.insert(5);
   C3.insert(6);
   C3.insert(7);
   C3.insert(8);
   C3.insert(9);
   C3.insert(10);
   //   C3.insert(11);
   
   auto mdd = new MDDRelax(cp,relaxSize);
   auto adv1 = all(cp, C1, [&Vars](int i) {return Vars[i];});
   Factory::allDiffMDD(mdd->getSpec(),adv1);
   cp->post(Factory::allDifferent(adv1));

   auto adv2 = all(cp, C2, [&Vars](int i) {return Vars[i];});
   Factory::allDiffMDD(mdd->getSpec(),adv2);
   cp->post(Factory::allDifferent(adv2));

   auto adv3 = all(cp, C3, [&Vars](int i) {return Vars[i];});
   Factory::allDiffMDD(mdd->getSpec(),adv3);
   cp->post(Factory::allDifferent(adv3));

   // All added to single MDD
   if (useMDD != 0) {
     cp->post(mdd);
   }
   

   DFSearch search(cp,[=]() {
       auto x = selectMin(Vars,
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
      
   search.onSolution([&Vars]() {
       std::cout << "Assignment:" << std::endl;
       std::cout << Vars << std::endl;
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
