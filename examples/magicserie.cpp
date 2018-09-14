#include <iostream>
#include <iomanip>
#include "solver.hpp"
#include "trailable.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"

int main(int argc,char* argv[])
{
    using namespace std;
    using namespace Factory;
    const int n = 50;
    CPSolver::Ptr cp  = Factory::makeSolver();
    auto s = Factory::intVarArray(cp,n,n);
    for(int i=0;i < n;i++) {
        cp->post(sum(Factory::intVarArray(cp,n,[&s,i](int j) { return Factory::isEqual(s[j],i);}),s[i]));
    }
    cp->post(sum(s,n));
    cp->post(sum(Factory::intVarArray(cp,n,[&s](int i) { return s[i]*i;}),n));
    
    DFSearch search(cp,[=]() {
                          auto x = selectMin(s,
                                             [](const auto& x) { return x->size() > 1;},
                                             [](const auto& x) { return x->size();});
                          if (x) {
                             int c = x->min();                    
                             return  [=] { cp->post(x == c);}
                                | [=] { cp->post(x != c);};
                          } else return Branches({});
                       });    

    search.onSolution([&s]() {
                         cout << "sol = " << s << endl;
                      });

    auto stat = search.solve();
    cout << stat << endl;    
    cp.dealloc();
    return 0;
}
