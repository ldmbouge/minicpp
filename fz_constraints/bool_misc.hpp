#pragma once

#include <intvar.hpp>
#include <fz_parser/flatzinc.h>

class bool2int : public Constraint
{
    protected:
        var<bool>::Ptr _a;
        var<int>::Ptr _b;

    public:
        bool2int(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars);
        void post() override;
        void propagate() override;
};

class bool_clause : public Constraint
{
    protected:
        std::vector<var<bool>::Ptr> _as;
        std::vector<var<bool>::Ptr> _bs;

    public:
        bool_clause(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars);
        void post() override;
        void propagate() override;
};