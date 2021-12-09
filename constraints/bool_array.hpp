#pragma once

#include <intvar.hpp>

class array_bool_and_reif : public Constraint
{
    protected:
        std::vector<var<bool>::Ptr> _as;
        var<bool>::Ptr _r;

    public:
        array_bool_and_reif(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const & vars, std::vector<int> const & consts);
        void post() override;
        void propagate() override;
};

class array_bool_element : public Constraint
{
    protected:
        var<int>::Ptr _b;
        std::vector<int> _as;
        var<bool>::Ptr _c;

    public:
        array_bool_element(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const & vars, std::vector<int> const & consts);
        void post() override;
        void propagate() override;
};

class array_bool_or_reif : public Constraint
{
    protected:
        std::vector<var<bool>::Ptr> _as;
        var<bool>::Ptr _r;

    public:
        array_bool_or_reif(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const & vars, std::vector<int> const & consts);
        void post() override;
        void propagate() override;
};

class array_bool_xor : public Constraint
{
    protected:
        std::vector<var<bool>::Ptr> _as;

    public:
        array_bool_xor(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const & vars, std::vector<int> const & consts);
        void post() override;
        void propagate() override;
};

class array_var_bool_element : public Constraint
{
    protected:
        var<int>::Ptr _b;
        std::vector<var<bool>::Ptr> _as;
        var<bool>::Ptr _c;

    public:
        array_var_bool_element(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const & vars, std::vector<int> const & consts);
        void post() override;
        void propagate() override;
};