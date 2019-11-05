#include <iostream>
#include <iomanip>
#include "bitset.hpp"
#include "solver.hpp"
#include "trailable.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"
#include "literal.hpp"

int main() {
   using namespace std;
   using namespace Factory;
   CPSolver::Ptr cp  = Factory::makeSolver();

    int n = 3;
    auto q = Factory::intVarArray(cp,n,1,n);

   //  add literal here
    auto l = Factory::makeLitVarEQ(q[0],3);
    l->setTrue();
   auto l2 = LitVarEQ<char>(q[0],2);
   cout << "is [[x1 == 2]]? A: " <<  (l2.isTrue() ? "YES" : "NO") << endl;



    DFSearch search(cp,[=]() {
                          auto x = selectMin(q,
                                             [](const auto& x) { return x->size() > 1;},
                                             [](const auto& x) { return x->size();});
                          if (x) {
                             int c = x->min();                    
                             return  [=] { cp->post(x == c);}
                                | [=] { cp->post(x != c);};
                          } else return Branches({});
                       });    
    search.onSolution([&q]() {
                         std::cout << "sol = " << q << std::endl;
                      });

    auto stat = search.solve();
    std::cout << stat << std::endl;
   //  std::cout << cp << std::endl;
    cp.dealloc();
    return 0;
}
