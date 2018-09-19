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

    const int n = 5;
    const int sumResult = n * (n * n + 1) / 2;
    
    CPSolver::Ptr cp  = Factory::makeSolver();
    matrix<var<int>::Ptr,2> x({n,n});
    for(int i=0;i < n;i++)
       for(int j=0;j < n;j++)
          x[i][j] = Factory::makeIntVar(cp,1,n*n);

    cp->post(Factory::allDifferent(x.flat()));    
    for(int i=0;i<n;i++)
       cp->post(sum(slice<var<int>::Ptr>(0,n,[i,&x](int j) { return x[i][j];}),sumResult));
    for(int j=0;j<n;j++)
       cp->post(sum(slice<var<int>::Ptr>(0,n,[j,&x](int i) { return x[i][j];}),sumResult));
    cp->post(sum(slice<var<int>::Ptr>(0,n,[&x](int i) { return x[i][i];}),sumResult));
    cp->post(sum(slice<var<int>::Ptr>(0,n,[n,&x](int i) { return x[n-i-1][i];}),sumResult));

    cp->post(x[0][n-1] <= x[n-1][0] - 1);
    cp->post(x[0][0] <= x[n-1][n-1] - 1);
    cp->post(x[0][0] <= x[n-1][0] - 1);
    
    
    DFSearch search(cp,firstFail(cp,x.flat()));

    auto stat = search.solve([](const SearchStatistics& stats) {
                                return stats.numberOfSolutions() >= 10000;
                             });
    cout << stat << endl;    
    cp.dealloc();
    return 0;
}
