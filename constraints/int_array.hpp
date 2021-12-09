#pragma once

#include <intvar.hpp>

class array_int_element : public Constraint
{
    protected:
        var<int>::Ptr _b;
        std::vector<int> _as;
        var<int>::Ptr _c;

    public:
        array_int_element(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const & vars, std::vector<int> const & consts);
        void post() override;
        void propagate() override;
};

class array_int_maximum : public Constraint
{
    protected:
        var<int>::Ptr _m;
        std::vector<var<int>::Ptr> _x;

    public:
        array_int_maximum(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const & vars, std::vector<int> const & consts);
        void post() override;
        void propagate() override;
};

class array_int_minimum : public Constraint
{
    protected:
        var<int>::Ptr _m;
        std::vector<var<int>::Ptr> _x;

    public:
        array_int_minimum(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const & vars, std::vector<int> const & consts);
        void post() override;
        void propagate() override;
};

class array_var_int_element : public Constraint
{
    protected:
        var<int>::Ptr _b;
        std::vector<var<int>::Ptr> _as;
        var<int>::Ptr _c;

    public:
        array_var_int_element(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const & vars, std::vector<int> const & consts);
        void post() override;
        void propagate() override;
};