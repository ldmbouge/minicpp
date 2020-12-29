/*
 * mini-cp is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License  v3
 * as published by the Free Software Foundation.
 *
 * mini-cp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY.
 * See the GNU Lesser General Public License  for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with mini-cp. If not, see http://www.gnu.org/licenses/lgpl-3.0.en.html
 *
 * Copyright (c)  2018. by Laurent Michel, Pierre Schaus, Pascal Van Hentenryck
 */

#include <iostream>
#include <iomanip>
#include "solver.hpp"
#include "trailable.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"

#define myassert(P) { if (!(P)) { printf("[%s]\n",#P);exit(2);}}

void test1() {
    using namespace std;
    using namespace Factory;
    CPSolver::Ptr cp  = Factory::makeSolver();
    auto x = Factory::intVarArray(cp,5,5);
    cp->post(Factory::allDifferentAC(x));
    cp->post(x[0] == 0);
    for(int i=1;i < 5;i++) {
       myassert(x[i]->size() == 4);
       myassert(x[i]->min()==1);
    }
}

void test2() {
    using namespace std;
    using namespace Factory;
    CPSolver::Ptr cp  = Factory::makeSolver();
    auto x = Factory::intVarArray(cp,5,5);
    cp->post(Factory::allDifferentAC(x));

    DFSearch search(cp,firstFail(cp,x));
    auto stats = search.solve();
    myassert(stats.numberOfSolutions()==120);
}

void test3() {
    using namespace std;
    using namespace Factory;
    CPSolver::Ptr cp  = Factory::makeSolver();
    auto x = Factory::intVarArray(cp,3);
    x[0] = Factory::makeIntVar(cp,{1,2});
    x[1] = Factory::makeIntVar(cp,{1,2});
    x[2] = Factory::makeIntVar(cp,{1,2,3,4});
    cout << "x[0] = " << x[0] << endl;
    cout << "x[1] = " << x[1] << endl;
    cout << "x[2] = " << x[2] << endl;
    cp->post(Factory::allDifferentAC(x));
    cout << "x[0] = " << x[0] << endl;
    cout << "x[1] = " << x[1] << endl;
    cout << "x[2] = " << x[2] << endl;
    myassert(x[2]->min() == 3);
    myassert(x[2]->size() == 2);
}

void test5() {
    using namespace std;
    using namespace Factory;
    CPSolver::Ptr cp  = Factory::makeSolver();
    auto x = Factory::intVarArray(cp,9);
    x[0] = Factory::makeIntVar(cp,{1,2,3,4,5});
    x[1] = Factory::makeIntVar(cp,{2});
    x[2] = Factory::makeIntVar(cp,{1,2,3,4,5});
    x[3] = Factory::makeIntVar(cp,{1});
    x[4] = Factory::makeIntVar(cp,{1,2,3,4,5,6});
    x[5] = Factory::makeIntVar(cp,{6,7,8});
    x[6] = Factory::makeIntVar(cp,{3});
    x[7] = Factory::makeIntVar(cp,{6,7,8,9});
    x[8] = Factory::makeIntVar(cp,{6,7,8});
    cp->post(Factory::allDifferentAC(x));
    myassert(x[0]->size()==2);
    myassert(x[2]->size()==2);
    myassert(x[4]->min()==6);
    myassert(x[7]->min()==9);
    myassert(x[8]->min()==7);
    myassert(x[8]->max()==8);
}

void test6() {
    using namespace std;
    using namespace Factory;
    CPSolver::Ptr cp  = Factory::makeSolver();
    auto x = Factory::intVarArray(cp,9);
    x[0] = Factory::makeIntVar(cp,{1,2,3,4,5});
    x[1] = Factory::makeIntVar(cp,{2,7});
    x[2] = Factory::makeIntVar(cp,{1,2,3,4,5});
    x[3] = Factory::makeIntVar(cp,{1,3});
    x[4] = Factory::makeIntVar(cp,{1,2,3,4,5,6});
    x[5] = Factory::makeIntVar(cp,{6,7,8});
    x[6] = Factory::makeIntVar(cp,{3,4,5});
    x[7] = Factory::makeIntVar(cp,{6,7,8,9});
    x[8] = Factory::makeIntVar(cp,{6,7,8});
    cp->post(Factory::allDifferentAC(x));    
    DFSearch search(cp,firstFail(cp,x));
    auto stats = search.solve();
    myassert(stats.numberOfFailures()==0);
    myassert(stats.numberOfSolutions()==80);
    cout << stats << endl;
}

void test7() {
    using namespace std;
    using namespace Factory;
    CPSolver::Ptr cp  = Factory::makeSolver();
    auto x = Factory::intVarArray(cp,9);
    x[0] = Factory::makeIntVar(cp,{3,4});
    x[1] = Factory::makeIntVar(cp,{1});
    x[2] = Factory::makeIntVar(cp,{3,4});
    x[3] = Factory::makeIntVar(cp,{0});
    x[4] = Factory::makeIntVar(cp,{3,4,5});
    x[5] = Factory::makeIntVar(cp,{5,6,7});
    x[6] = Factory::makeIntVar(cp,{2,9,10});
    x[7] = Factory::makeIntVar(cp,{5,6,7,8});
    x[8] = Factory::makeIntVar(cp,{5,6,7});
    cp->post(Factory::allDifferentAC(x));

    myassert(!x[4]->contains(3));
    myassert(!x[4]->contains(4));
    myassert(!x[5]->contains(5));
    myassert(!x[7]->contains(5));
    myassert(!x[7]->contains(6));
    myassert(!x[8]->contains(5));
}

void test8() {
    using namespace std;
    using namespace Factory;
    CPSolver::Ptr cp  = Factory::makeSolver();
    auto x = Factory::intVarArray(cp,5);
    x[0] = Factory::makeIntVar(cp,{0,2,3,5});
    x[1] = Factory::makeIntVar(cp,{4});
    x[2] = Factory::makeIntVar(cp,{-1,1});
    x[3] = Factory::makeIntVar(cp,{-4,-2,0,2,3});
    x[4] = Factory::makeIntVar(cp,{-1});
    cp->post(Factory::allDifferentAC(x));

    myassert(!x[2]->contains(-1));
}

int main(int argc,char* argv[])
{
   test1();
   test2();
   test3();
   test5();
   test6();
   test7();
   test8();
   return 0;
}
