#pragma once

#include <intvar.hpp>

class bool_lin : public Constraint
{
    protected:
        std::vector<int> _as_pos;
        std::vector<int> _as_neg;
        std::vector<var<bool>::Ptr> _bs_pos;
        std::vector<var<bool>::Ptr> _bs_neg;
        int _c;

    public:
        bool_lin(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const & vars, std::vector<int> const & consts);
        static void calSumMinMax(bool_lin* boolLin, int& sumMin, int& sumMax);
        void post() override;

    friend class bool_lin_ge;
    friend class bool_lin_le;
};

class bool_lin_eq : public bool_lin
{
    public:
        bool_lin_eq(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const & vars, std::vector<int> const & consts);
        void post() override;
        void propagate() override;
};

class bool_lin_ge
{
    public:
        static void propagate(bool_lin* boolLin, int sumMin, int sumMax);
};

class bool_lin_le : public bool_lin
{
    public:
        bool_lin_le(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const & vars, std::vector<int> const & consts);
        void post() override;
        void propagate() override;
        static void propagate(bool_lin* boolLin, int sumMin, int sumMax);
};