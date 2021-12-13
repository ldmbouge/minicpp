#pragma once

#include <intvar.hpp>

class int_tern : public Constraint
{
    protected:
        var<int>::Ptr _a;
        var<int>::Ptr _b;
        var<int>::Ptr _c;

    public:
        int_tern(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const & vars, std::vector<int> const & consts);
        static void calMulMinMax(int aMin, int aMax, int bMin, int bMax, int& min, int& max);
        static void calDivMinMax(int aMin, int aMax, int bMin, int bMax, int& min, int& max);
        void post() override;
};

class int_div : public int_tern
{
    public:
        int_div(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const & vars, std::vector<int> const & consts);
        void post() override;
        void propagate() override;
};

class int_max : public int_tern
{
    public:
        int_max(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const & vars, std::vector<int> const & consts);
        void post() override;
        void propagate() override;
};

class int_min : public int_tern
{
    public:
        int_min(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const & vars, std::vector<int> const & consts);
        void post() override;
        void propagate() override;
};

class int_mod : public int_tern
{
    public:
        int_mod(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const & vars, std::vector<int> const & consts);
        void post() override;
        void propagate() override;
};

class int_plus : public int_tern
{
    public:
        int_plus(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const & vars, std::vector<int> const & consts);
        void post() override;
        void propagate() override;
};

class int_pow : public int_tern
{
    public:
        int_pow(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const & vars, std::vector<int> const & consts);
        void post() override;
        void propagate() override;
    private:
        static void calcPowMinMax(int aMin, int aMax, int bMin, int bMax, int& min, int& max);
        static void calcPowMinMax(int aMin, int aMax, double bVal, int& min, int& max);
};

class int_times : public int_tern
{
    public:
        int_times(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const & vars, std::vector<int> const & consts);
        void post() override;
        void propagate() override;
};