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

   //  test 1
    int n = 2;
    auto q = Factory::intVarArray(cp,n,1,3);
    auto l1 = Factory::makeLitVarNEQ(q[0],1);
    auto l2 = Factory::makeLitVarLEQ(q[1],2);
    auto allLits = vector<LitVar::Ptr>({l1,l2});

   //  test 2
   //  int n = 3;
   //  auto q = Factory::intVarArray(cp,n,1,2);
   //  auto l1 = Factory::makeLitVarLEQ(q[0],1);
   //  auto l2 = Factory::makeLitVarGEQ(q[1],2);
   //  auto l3 = Factory::makeLitVarNEQ(q[1],2);
   //  auto allLits = vector<LitVar::Ptr>({l1,l2,l3});




    cp->post(Factory::litClause(allLits));


   //  DFSearch search(cp,[=]() {
   //                        auto x = selectUnboundLit(allLits);
   //                        if (x) {
   //                           return  [=] { cp->post(isTrue(x));}
   //                              | [=] { cp->post(isFalse(x));};
   //                        } else return Branches({});
   //                     });    
    
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
