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
#include "solver.hpp"
#include "trailable.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"
#include <flatzinc/flatzinc.h>
#include "constraint_flatzinc.hpp"

void vec2str(std::vector<int> const * const v, std::string * s,  char const brackets[2] = "[]")
{
    const int size = v->size();
    *s = brackets[0];
    for (int i = 0; i < size; i += 1)
    {
        const int t = v->at(i);
        *s += std::to_string(t);
        *s += (i < size - 1) ? "," : "";
    }
    *s += brackets[1];
}

int main(int argc,char* argv[])
{
    using namespace std;
    using namespace Factory;

    //Parsing FlatZinc
    FlatZinc::Printer p;
    FlatZinc::FlatZincModel* fm = FlatZinc::parse(argv[1], p);

    //Instantiate the problem
    CPSolver::Ptr cp  = Factory::makeSolver();
    std::vector<var<int>::Ptr>* intVars = new std::vector<var<int>::Ptr>();
    std::vector<var<bool>::Ptr>* boolVars = new std::vector<var<bool>::Ptr>();

    printf("===== VARIABLES =====\n");
    //Int variables
    for(size_t i = 0; i < fm->int_vars.size(); i += 1)
    {
        FlatZinc::IntVar iv_fzn = fm->int_vars[i];
        var<int>::Ptr iv_cp;
        printf("%lu int: ", i);
        if (iv_fzn.interval)
        {
            iv_cp = Factory::makeIntVar(cp, iv_fzn.min, iv_fzn.max);
            printf("{%d,...,%d}\n", iv_fzn.min, iv_fzn.max);
        }
        else
        {
            iv_cp = Factory::makeIntVar(cp, iv_fzn.values);
            string domain;
            vec2str(&iv_fzn.values, &domain, "{}");
            printf("%s\n", domain.c_str());
        }
        intVars->push_back(iv_cp);
    }

    //Bool variables
    for(size_t i = 0; i < fm->bool_vars.size(); i += 1)
    {
        FlatZinc::BoolVar bv_fzn = fm->bool_vars[i];
        var<bool>::Ptr bv_cp;
        printf("%lu bool: ", i);
        if (bv_fzn.state == FlatZinc::BoolVar::Unassigned)
        {
            bv_cp = Factory::makeBoolVar(cp);
            printf("{false,true}");
        }
        else
        {
            bool value = bv_fzn.state == FlatZinc::BoolVar::True;
            bv_cp = Factory::makeBoolVar(cp, value);
            printf(value ? "{true}" : "{false}");
        }
        printf("\n");

        boolVars->push_back(bv_cp);
    }

    // Constraints
    printf("===== CONSTRAINTS =====\n");
    for(size_t i = 0; i < fm->constraints.size(); i += 1)
    {
        FlatZinc::Constraint c = fm->constraints[i];
        cp->post(FznConstraint(cp, intVars, boolVars, c));

        std::string vars_str;
        std::string consts_str;
        vec2str(&c.vars, &vars_str);
        vec2str(&c.consts, &consts_str);
        printf("%lu %s: Variables %s | Constants %s\n", i, FlatZinc::Constraint::type2str[c.type], vars_str.c_str(), consts_str.c_str());
    }

    //Search
    printf("===== SEARCH =====\n");
    printf("Problem: %s", FlatZinc::problem2str[fm->problem]);
    if(fm->problem == FlatZinc::Problem::Satisfaction)
    {
        printf("\n");
    }
    else
    {
        printf(" (Objective var %d)\n", fm->objective_variable);
    }
    std::string vars_str;
    vec2str(&fm->decisionVariables, &vars_str);
    printf("Decision variables: %s\n", vars_str.c_str());
    printf("Variable selection: %s\n", FlatZinc::varSel2str[fm->variableSelection]);
    printf("Value selection: %s\n", FlatZinc::valSel2str[fm->valueSelection]);

    Objective::Ptr obj = Factory::minimize(intVars->at(fm->objective_variable));
    std::vector<var<int>::Ptr> decisionVariables;
    for(auto i: fm->decisionVariables)
    {
        decisionVariables.push_back(intVars->at(i));
    }
    DFSearch search(cp,firstFail(cp,decisionVariables));

    search.onSolution([&obj, &decisionVariables]() {
        cout << "solution = ";
        for(size_t i  = 0; i < decisionVariables.size(); i += 1)
        {
            cout << decisionVariables[i]->min() << " ";
        }
        cout << endl << "objective = " << obj->value() << endl;
        std::cout << "-----" <<endl;});

    search.onFailure([&decisionVariables]() {
        /*
        cout << "Fail = ";
        for(size_t i  = 0; i < decisionVariables.size(); i += 1)
        {
            auto v = decisionVariables[i];
            if (v->isBound())
            {
                cout << v->min();
            }
            else
            {
                cout << "[" <<  v->min() << "," << v->max() << "]";
            }
            cout << " ";
        }
        cout << endl;
         */
    });


    auto stat = search.optimize(obj);
    cout << stat << endl;
    cp.dealloc();
    return 0;


    /*
    CPSolver::Ptr cp  = Factory::makeSolver();
    const int n = argc >= 2 ? atoi(argv[1]) : 12;
    const bool one = argc >= 3 ? atoi(argv[2])==0 : false;
    auto q = Factory::intVarArray(cp,n,1,n);
    for(int i=0;i < n;i++)
        for(int j=i+1;j < n;j++) {
            cp->post(q[i] != q[j]);            
            cp->post(Factory::notEqual(q[i],q[j],i-j));            
            cp->post(Factory::notEqual(q[i],q[j],j-i));            
        }
    Objective::Ptr obj = Factory::minimize(q[n-1]);
    
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

    int nbSol = 0;
    search.onSolution([&nbSol,&q]() {
                         cout << "sol = " << q << endl;
                         nbSol++;
                      });
    auto stat = search.optimize(obj,[one,&nbSol](const SearchStatistics& stats) {
                                       return one ? nbSol > 1 : false;
                                    });
   
    cout << "Got: " << nbSol << " solutions" << endl;
    cout << stat << endl;
    cp.dealloc();
    return 0;
     */
}
