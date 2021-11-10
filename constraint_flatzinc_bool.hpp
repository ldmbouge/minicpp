#pragma once

#include "solver.hpp"
#include "intvar.hpp"

class array_bool_or_reif : public Constraint
{
    std::vector<var<bool>::Ptr> _as;
    var<bool>::Ptr _r;

    public:
    array_bool_or_reif(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const & vars, std::vector<int> const & consts);
    void post() override;
    void propagate() override;
};


class bool_clause : public Constraint
{
    std::vector<var<bool>::Ptr> _as;
    std::vector<var<bool>::Ptr> _bs;

    public:
    bool_clause(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const & vars, std::vector<int> const & consts);
    void post() override;
    void propagate() override;
};
