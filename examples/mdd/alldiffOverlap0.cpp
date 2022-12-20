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
#include <cstring>


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

void buildModel(CPSolver::Ptr cp, int relaxSize, int mode)
{
   // mode: 0 (standard alldiff), 1 (AC alldiff), 2 (MDD), 3 (AC + MDD)

   using namespace std;

   // Settings as in Andersen et al. [CP 2007]:
   // Three overlapping Alldiffs with parameters (n,r,d) where
   //   n = number of variables
   //   r = size of scope of each constraint
   //   d = domain size
   // The paper reports (10,9,9) and (12,10,10), e.g.,
   //
   //   set<int> C1 = {0,1,  3,4,5,6,7,8,  10,11};
   //   set<int> C2 = {0,1,2,3,  5,6,  8,9,10,11};
   //   set<int> C3 = {0,1,2,  4,5,6,7,8,9,10};
   //
   // Issue: the "Hall set" check in MDD makes these instances trivial,
   // which means we need should compare with AC-alldiff.  We therefore 
   // consider more complex instances with more alldiff constraints (and
   // more variables).

   int N = 15;
   int D = 0; // set later, to minimum Alldiff scope
   int NbCliques = 9;

   // initialize random seed
   srand(1234);
   

   vector< set<int> > Cliques;
   /*** This part was used to generate random cliques ***
   for (int i=0; i<NbCliques; i++) {
     set<int> C;
     // for (int j=0; j<N; j++) {
     //   // include variable with some probability 
     //   auto v = rand() % 100;
     //   if (v < 60) {
     // 	 C.insert(j);
     //   }
     // }
     for (int j=i; j<=i+N-NbCliques; j++) {
       // include variables from staggered interval (best case scenario for MDDs)
       auto v = rand() % 100;
       if (v < 60) {
     	 C.insert(j);
       }
     }
     
     std::cout << "clique " << i << ": " << C << std::endl;
     Cliques.push_back(C);

     if (D < (int)C.size()) D = (int)C.size();
   }   
   ***/

   // Hard-coded instance
   set<int> C = {0,1,2,5,6};
   Cliques.push_back(C);
   C =  {3,5,7};
   Cliques.push_back(C);
   C =  {4,5,7,8};
   Cliques.push_back(C);
   C =  {3,4,6,8,9};
   Cliques.push_back(C);
   C =  {4,5,7};
   Cliques.push_back(C);
   C =  {6,7,9,10};
   Cliques.push_back(C);
   C =  {7,10,11,12};
   Cliques.push_back(C);
   C =  {7,8,9,11};
   Cliques.push_back(C);
   C =  {8,9,10,13};
   Cliques.push_back(C);

   D = 5;
   
   std::cout << "N = " << N << std::endl;
   std::cout << "D = " << D << std::endl;
   std::cout << "NbCliques = " << NbCliques << std::endl;      

   auto vars = Factory::intVarArray(cp, N, 0, D-1);
   MDDRelax* mdd = nullptr;
   if (mode == 0) {
     for (int i=0; i<NbCliques; i++) {
       auto adv = all(cp, Cliques[i], [&vars](int i) {return vars[i];});
       cp->post(Factory::allDifferent(adv));
     }
   }
   if ((mode == 1) || (mode == 3)) {
     for (int i=0; i<NbCliques; i++) {
       auto adv = all(cp, Cliques[i], [&vars](int i) {return vars[i];});
       cp->post(Factory::allDifferentAC(adv));
     }
   }
   if ((mode == 2) || (mode==3)) {
     mdd = new MDDRelax(cp,relaxSize);
     for (int i=0; i<NbCliques; i++) {
       auto adv = all(cp, Cliques[i], [&vars](int i) {return vars[i];});
       mdd->post(Factory::allDiffMDD(mdd,adv));
     }
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
         //int i = x->getId();
          
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
   int width = (argc >= 2 && strncmp(argv[1],"-w",2)==0) ? atoi(argv[1]+2) : 1;
   int mode  = (argc >= 3 && strncmp(argv[2],"-m",2)==0) ? atoi(argv[2]+2) : 0;

   // mode: 0 (standard alldiff), 1 (AC alldiff), 2 (MDD), 3 (AC + MDD)
   
   std::cout << "width = " << width << std::endl;
   std::cout << "mode = " << mode << std::endl;
   try {
      CPSolver::Ptr cp  = Factory::makeSolver();
      buildModel(cp, width, mode);
   } catch(Status s) {
      std::cout << "model infeasible during post" << std::endl;
   } catch (std::exception& e) {
      std::cerr << "Unable to find the file" << std::endl;
   }

   return 0;   
}
