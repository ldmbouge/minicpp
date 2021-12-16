#include <utils.hpp>
#include <constraints/bool_lin.hpp>

bool_lin::bool_lin(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars) :
    Constraint(cp),
    _as_pos(),
    _as_neg(),
    _bs_pos(),
    _bs_neg(),
    _c(fzConstraint.consts.back())
{
    for(size_t i = 0; i < fzConstraint.consts.size() - 1; i += 1)
    {
        if(fzConstraint.consts[i] > 0)
        {
            _as_pos.push_back(fzConstraint.consts[i]);
            _bs_pos.push_back(bool_vars[fzConstraint.vars[i]]);
        }
        else
        {
            _as_neg.push_back(fzConstraint.consts[i]);
            _bs_neg.push_back(bool_vars[fzConstraint.vars[i]]);
        }
    }
}

void bool_lin::calSumMinMax(bool_lin* boolLin, int& min, int& max)
{
    auto& _as_pos = boolLin->_as_pos;
    auto& _as_neg = boolLin->_as_neg;
    auto& _bs_pos = boolLin->_bs_pos;
    auto& _bs_neg = boolLin->_bs_neg;

    min = 0;
    max = 0;
    for (size_t i = 0; i < _as_pos.size(); i += 1)
    {
        min += _as_pos[i] * _bs_pos[i]->min();
        max += _as_pos[i] * _bs_pos[i]->max();
    }
    for (size_t i = 0; i < _as_neg.size(); i += 1)
    {
        min += _as_neg[i] * _bs_neg[i]->max();
        max += _as_neg[i] * _bs_neg[i]->min();
    }
}

void bool_lin::post()
{
    for(auto x : _bs_pos)
    {
        x->propagateOnBind(this);
    }

    for(auto x : _bs_neg)
    {
        x->propagateOnBind(this);
    }
}


bool_lin_eq::bool_lin_eq(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars) :
    bool_lin(cp, fzConstraint, int_vars, bool_vars)
{}

void bool_lin_eq::post()
{
    bool_lin::post();
    propagate();
}

void bool_lin_eq::propagate()
{
    //Semantic: as1*bs1 + ... + asn*bsn = c
    int sumMin;
    int sumMax;
    calSumMinMax(this, sumMin, sumMax);

    //Propagation: as1*bs1 + ... + asn*bsn <- c
    bool_lin_ge::propagate(this, sumMin, sumMax);
    bool_lin_le::propagate(this, sumMin, sumMax);
}

void bool_lin_ge::propagate(bool_lin* boolLin, int sumMin, int sumMax)
{
    //Semantic: as1*bs1 + ... + asn*bsn >= c
    auto& _as_pos = boolLin->_as_pos;
    auto& _as_neg = boolLin->_as_neg;
    auto& _bs_pos = boolLin->_bs_pos;
    auto& _bs_neg = boolLin->_bs_neg;
    auto& _c = boolLin->_c;

    //Propagation: as1*bs1 + ... + asn*bsn <- c
    for (size_t i = 0; i < _as_pos.size(); i += 1)
    {
        int iMin = _as_pos[i] * _bs_pos[i]->min();
        int iMax = _as_pos[i] * _bs_pos[i]->max();
        if (iMax - iMin > -(_c - sumMax))
        {
            int bsMin = ceilDivision(_c - sumMax + iMax, _as_pos[i]);
            _bs_pos[i]->removeBelow(bsMin);
            sumMin = sumMin - iMin + _as_pos[i]*bsMin;
        }
    }
    for (size_t i = 0; i < _as_neg.size(); i += 1)
    {
        int iMin = _as_neg[i] * _bs_neg[i]->max();
        int iMax = _as_neg[i] * _bs_neg[i]->min();
        if (iMax - iMin > -(_c - sumMax))
        {
            int bsMax = floorDivision(-(_c - sumMax + iMax), -_as_neg[i]);
            _bs_neg[i]->removeAbove(bsMax);
            sumMin = sumMin - iMin + _as_neg[i]*bsMax;
        }
    }
}

bool_lin_le::bool_lin_le(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars) :
    bool_lin(cp, fzConstraint, int_vars, bool_vars)
{}

void bool_lin_le::post()
{
    bool_lin::post();
    propagate();
}

void bool_lin_le::propagate()
{
    int sumMin;
    int sumMax;
    calSumMinMax(this, sumMin, sumMax);
    propagate(this, sumMin, sumMax);
}

void bool_lin_le::propagate(bool_lin* boolLin, int sumMin, int sumMax)
{
    //Semantic: as1*bs1 + ... + asn*bsn <= c
    auto& _as_pos = boolLin->_as_pos;
    auto& _as_neg = boolLin->_as_neg;
    auto& _bs_pos = boolLin->_bs_pos;
    auto& _bs_neg = boolLin->_bs_neg;
    auto& _c = boolLin->_c;

    //Propagation: as1*bs1 + ... + asn*bsn <- c
    for (size_t i = 0; i < _as_pos.size(); i += 1)
    {
        int iMin = _as_pos[i] * _bs_pos[i]->min();
        int iMax = _as_pos[i] * _bs_pos[i]->max();

        if (iMax - iMin > _c - sumMin)
        {
            int bsMax = floorDivision(_c - sumMin + iMin, _as_pos[i]);
            _bs_pos[i]->removeAbove(bsMax);
            sumMax = sumMax - iMax + _as_pos[i]*bsMax;
        }
    }
    for (size_t i = 0; i < _as_neg.size(); i += 1)
    {
        int iMin = _as_neg[i] * _bs_neg[i]->max();
        int iMax = _as_neg[i] * _bs_neg[i]->min();
        if (iMax - iMin > _c - sumMin)
        {
            int bsMin = ceilDivision(-(_c - sumMin + iMin), -_as_neg[i]);
            _bs_neg[i]->removeBelow(bsMin);
            sumMax = sumMax - iMax + _as_neg[i]*bsMin;
        }
    }
}