#pragma once

#include <intvar.hpp>
#include <fz_parser/flatzinc.h>

class array_bool : public Constraint
{
    protected:
        std::vector<var<bool>::Ptr> _as;

    public:
        array_bool(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars);
        void post() override;
};

class array_bool_reif : public array_bool
{
    protected:
        var<bool>::Ptr _r;

    public:
        array_bool_reif(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars);
        void post() override;
};

class array_bool_and_imp : public array_bool_reif
{
public:
    array_bool_and_imp(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars);
    void post() override;
    void propagate() override;
};

class array_bool_and_reif : public array_bool_reif
{
    public:
        array_bool_and_reif(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars);
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
        array_bool_element(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars);
        void post() override;
        void propagate() override;
};

class array_bool_or_imp : public array_bool_reif
{
public:
    array_bool_or_imp(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars);
    void post() override;
    void propagate() override;
};

class array_bool_or_reif : public array_bool_reif
{
    public:
        array_bool_or_reif(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars);
        void post() override;
        void propagate() override;
};

class array_bool_xor : public array_bool
{
    public:
        array_bool_xor(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars);
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
        array_var_bool_element(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars);
        void post() override;
        void propagate() override;
};