#pragma once

#include <intvar.hpp>
#include <fz_parser/flatzinc.h>

class bool_lin : public Constraint
{
    protected:
        std::vector<int> _as_pos;
        std::vector<int> _as_neg;
        std::vector<var<bool>::Ptr> _bs_pos;
        std::vector<var<bool>::Ptr> _bs_neg;
        int _c;
        int _sumMin;
        int _sumMax;

    public:
        bool_lin(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars);
        static void calSumMinMax(bool_lin* bl);
        void post() override;

    friend class bool_lin_ge;
    friend class bool_lin_le;
};

class bool_lin_eq : public bool_lin
{
    public:
        bool_lin_eq(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars);
        void post() override;
        void propagate() override;
};

class bool_lin_ge
{
    public:
        static void propagate(bool_lin* bl);
};

class bool_lin_le : public bool_lin
{
    public:
        bool_lin_le(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars);
        void post() override;
        void propagate() override;
        static void propagate(bool_lin* bl);
};