#pragma once

#include <flatzinc/flatzinc.h>
#include "intvar.hpp"

class array_int_element : public Constraint
{
   var<int>::Ptr _b;
   std::vector<int> _as;
   var<int>::Ptr _c;

    public:
    array_int_element(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const & vars, std::vector<int> const & consts);
    void post() override;
    void propagate() override;
};

class array_var_int_element : public Constraint
{
    var<int>::Ptr _b;
    std::vector<var<int>::Ptr> _as;
    var<int>::Ptr _c;

    public:
    array_var_int_element(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const & vars, std::vector<int> const & consts);
    void post() override;
    void propagate() override;
};

class int_eq_reif : public Constraint
{
    var<int>::Ptr _a;
    var<int>::Ptr _b;
    var<bool>::Ptr _r;

    public:
    int_eq_reif(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const & vars, std::vector<int> const & consts);
    void post() override;
    void propagate() override;
};

class int_lin_eq : public Constraint
{
    std::vector<int> _as;
    std::vector<var<int>::Ptr> _bs;
    int _c;

    public:
    int_lin_eq(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const & vars, std::vector<int> const & consts);
    void post() override;
    void propagate() override;
    static void propagate(std::vector<int>& _as, std::vector<var<int>::Ptr>& _bs, int c);
};

class int_lin_eq_reif : public Constraint
{
    std::vector<int> _as;
    std::vector<var<int>::Ptr> _bs;
    int _c;
    var<bool>::Ptr _r;

    public:
    int_lin_eq_reif(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const & vars, std::vector<int> const & consts);
    void post() override;
    void propagate() override;
};

class int_lin_ne : public Constraint
{
    std::vector<int> _as;
    std::vector<var<int>::Ptr> _bs;
    int _c;

    public:
    int_lin_ne(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const & vars, std::vector<int> const & consts);
    void post() override;
    void propagate() override;
    static void propagate(std::vector<int>& _as, std::vector<var<int>::Ptr>& _bs, int c);
};