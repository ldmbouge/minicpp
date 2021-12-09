#pragma once

#include <intvar.hpp>

class bool2int : public Constraint
{
    protected:
        var<bool>::Ptr _a;
        var<int>::Ptr _b;

    public:
        bool2int(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const & vars, std::vector<int> const & consts);
        void post() override;
        void propagate() override;
};

class bool_clause : public Constraint
{
    protected:
        std::vector<var<bool>::Ptr> _as;
        std::vector<var<bool>::Ptr> _bs;

    public:
        bool_clause(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const & vars, std::vector<int> const & consts);
        void post() override;
        void propagate() override;
};