#pragma once

#include <intvar.hpp>
#include <flatzinc/flatzinc.h>

class bool_bin : public Constraint
{
    protected:
        var<bool>::Ptr _a;
        var<bool>::Ptr _b;

    public:
        bool_bin(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars);
        void post() override;
};

class bool_bin_reif : public bool_bin
{
    protected:
        var<bool>::Ptr _r;

    public:
        bool_bin_reif(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars);
        void post() override;
};

class bool_and_reif : public bool_bin_reif
{
    public:
        bool_and_reif(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars);
        void post() override;
        void propagate() override;
};

class bool_eq : public bool_bin
{
    public:
        bool_eq(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars);
        void post() override;
        void propagate() override;
        static void propagate(Constraint* c, var<bool>::Ptr _a, var<bool>::Ptr _b);
};

class bool_eq_reif : public bool_bin_reif
{
    public:
        bool_eq_reif(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars);
        void post() override;
        void propagate() override;
};

class bool_le : public bool_bin
{
public:
    bool_le(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars);
    void post() override;
    void propagate() override;
    static void propagate(Constraint* c, var<bool>::Ptr _a, var<bool>::Ptr _b);
};

class bool_le_reif : public bool_bin_reif
{
public:
    bool_le_reif(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars);
    void post() override;
    void propagate() override;
};

class bool_lt : public bool_bin
{
public:
    bool_lt(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars);
    void post() override;
    void propagate() override;
    static void propagate(Constraint* c, var<bool>::Ptr _a, var<bool>::Ptr _b);

};

class bool_lt_reif : public bool_bin_reif
{
public:
    bool_lt_reif(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars);
    void post() override;
    void propagate() override;
};

class bool_not : public bool_bin
{
public:
    bool_not(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars);
    void post() override;
    void propagate() override;
    static void propagate(Constraint* c, var<bool>::Ptr _a, var<bool>::Ptr _b);
};

class bool_or_reif : public bool_bin_reif
{
public:
    bool_or_reif(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars);
    void post() override;
    void propagate() override;
};

class bool_xor : public bool_bin
{
public:
    bool_xor(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars);
    void post() override;
    void propagate() override;
};

class bool_xor_reif : public bool_bin_reif
{
public:
    bool_xor_reif(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars);
    void post() override;
    void propagate() override;
};