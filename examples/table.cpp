#include <iostream>
#include <iomanip>
#include "bitset.hpp"
#include "solver.hpp"
#include "trailable.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"

int main() {
   using namespace std;
   using namespace Factory;
   CPSolver::Ptr cp  = Factory::makeSolver();

//    SparseBitSet s(cp->getStateManager(), cp->getStore(), 40);
//    StaticBitSet t(40);
//    cout << s.isEmpty() << endl;
//    s.clearMask();
//    s.addToMask(t);
//    s.intersectWithMask();
//    cout << s.isEmpty() << endl;


    int n = 3;
    auto q = Factory::intVarArray(cp,n,1,n);
    std::vector<std::vector<int>> table;
    table.emplace_back(std::vector<int> {1,2,3});
    table.emplace_back(std::vector<int> {3,2,1});
    table.emplace_back(std::vector<int> {2,1,3});
    cp->post(Factory::table(q, table));
   //  cp->post(Factory::allDifferentAC(q));
   //  cp->post(q[0] == 4);
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
