#include <iostream>
#include <iomanip>
#include <fstream>
#include "solver.hpp"
#include "trailable.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"

int main(int argc,char* argv[])
{
    using namespace std;
    using namespace Factory;

    ifstream data("data/qap.txt");
    int n;
    data >> n;
    matrix<int,2> w({n,n});
    for(int i =0;i < n;i++)
       for(int j=0;j < n;j++)
          data >> w[i][j];
    matrix<int,2> d({n,n});
    for(int i =0;i < n;i++)
       for(int j=0;j < n;j++)
          data >> d[i][j];

    /*
    for(int i =0;i < n;i++) {
       for(int j=0;j < n;j++)
          cout << setw(3) << w[i][j] << " ";
       cout << endl;
    }
    for(int i =0;i < n;i++) {
       for(int j=0;j < n;j++)
          cout << setw(3) << d[i][j] << " ";
       cout << endl;
    }
    */

    
    
    CPSolver::Ptr cp  = Factory::makeSolver();
    auto x = Factory::intVarArray(cp,n,n);
    cp->post(Factory::allDifferent(x));
    auto wDist = Factory::intVarArray(cp,n*n);
    for(int k=0,i=0;i < n;i++)
       for(int j=0;j < n;j++)
          wDist[k++] = w[i][j] * Factory::element(d,x[i],x[j]);
    Objective::Ptr obj = Factory::minimize(Factory::sum(wDist));
       
    DFSearch search(cp,firstFail(cp,x));

    search.onSolution([&x,&obj]() {
                         cout << "objective = " << obj->value() << endl;
                      });

    auto stat = search.optimize(obj);
    cout << stat << endl;    
    cp.dealloc();
    return 0;
}
