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

    int n = 2;
    auto q = Factory::intVarArray(cp,n,1,2);

   //  add vector of literals here
    auto l1 = Factory::makeLitVarEQ(q[0],1);
    auto l2 = Factory::makeLitVarEQ(q[1],1);
   //  auto l3 = Factory::makeLitVarEQ(q[2],1);
    auto allLits = vector<LitVar::Ptr>({l1,l2});
    auto lits = vector<LitVar::Ptr>({l1,l2});
    cp->post(Factory::makeLitClause(lits));


    DFSearch search(cp,[=]() {
                          auto x = selectUnboundLit(allLits);
                          if (x) {
                             return  [=] { x->assign(true); cp->fixpoint();}
                                | [=] { x->assign(false); cp->fixpoint();};
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
