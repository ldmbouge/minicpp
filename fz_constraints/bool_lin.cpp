#include <utils.hpp>
#include <fz_constraints/bool_lin.hpp>

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

void bool_lin::calSumMinMax(bool_lin* bl)
{
    auto& _as_pos = bl->_as_pos;
    auto& _as_neg = bl->_as_neg;
    auto& _bs_pos = bl->_bs_pos;
    auto& _bs_neg = bl->_bs_neg;

    bl->_sumMin = 0;
    bl->_sumMax = 0;
    for (size_t i = 0; i < _as_pos.size(); i += 1)
    {
        bl->_sumMin += _as_pos[i] * _bs_pos[i]->min();
        bl->_sumMax += _as_pos[i] * _bs_pos[i]->max();
    }
    for (size_t i = 0; i < _as_neg.size(); i += 1)
    {
        bl->_sumMin += _as_neg[i] * _bs_neg[i]->max();
        bl->_sumMax += _as_neg[i] * _bs_neg[i]->min();
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
    calSumMinMax(this);

    //Propagation: as1*bs1 + ... + asn*bsn <- c
    bool_lin_ge::propagate(this);
}

void bool_lin_ge::propagate(bool_lin* bl)
{
    //Semantic: as1*bs1 + ... + asn*bsn >= c
    auto& _as_pos = bl->_as_pos;
    auto& _as_neg = bl->_as_neg;
    auto& _bs_pos = bl->_bs_pos;
    auto& _bs_neg = bl->_bs_neg;
    auto& _c = bl->_c;
    auto& _sumMin = bl->_sumMin;
    auto& _sumMax = bl->_sumMax;

    //Propagation: as1*bs1 + ... + asn*bsn <- c
    for (size_t i = 0; i < _as_pos.size(); i += 1)
    {
        int iMin = _as_pos[i] * _bs_pos[i]->min();
        int iMax = _as_pos[i] * _bs_pos[i]->max();
        if (iMax - iMin > -(_c - _sumMax))
        {
            int bsMin = ceilDivision(_c - _sumMax + iMax, _as_pos[i]);
            _bs_pos[i]->removeBelow(bsMin);
            _sumMin = _sumMin - iMin + _as_pos[i]*bsMin;
        }
    }
    for (size_t i = 0; i < _as_neg.size(); i += 1)
    {
        int iMin = _as_neg[i] * _bs_neg[i]->max();
        int iMax = _as_neg[i] * _bs_neg[i]->min();
        if (iMax - iMin > -(_c - _sumMax))
        {
            int bsMax = floorDivision(-(_c - _sumMax + iMax), -_as_neg[i]);
            _bs_neg[i]->removeAbove(bsMax);
            _sumMin = _sumMin - iMin + _as_neg[i]*bsMax;
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
    calSumMinMax(this);

    propagate(this);

    if(_sumMax <= _c)
    {
        setActive(false);
    }
    else if (_sumMin > _c)
    {
        failNow();
    }
}

void bool_lin_le::propagate(bool_lin* bl)
{
    //Semantic: as1*bs1 + ... + asn*bsn <= c
    auto& _as_pos = bl->_as_pos;
    auto& _as_neg = bl->_as_neg;
    auto& _bs_pos = bl->_bs_pos;
    auto& _bs_neg = bl->_bs_neg;
    auto& _c = bl->_c;
    auto& _sumMin = bl->_sumMin;
    auto& _sumMax = bl->_sumMax;

    //Propagation: as1*bs1 + ... + asn*bsn <- c
    for (size_t i = 0; i < _as_pos.size(); i += 1)
    {
        int iMin = _as_pos[i] * _bs_pos[i]->min();
        int iMax = _as_pos[i] * _bs_pos[i]->max();

        if (iMax - iMin > _c - _sumMin)
        {
            int bsMax = floorDivision(_c - _sumMin + iMin, _as_pos[i]);
            _bs_pos[i]->removeAbove(bsMax);
            _sumMax = _sumMax - iMax + _as_pos[i]*bsMax;
        }
    }
    for (size_t i = 0; i < _as_neg.size(); i += 1)
    {
        int iMin = _as_neg[i] * _bs_neg[i]->max();
        int iMax = _as_neg[i] * _bs_neg[i]->min();
        if (iMax - iMin > _c - _sumMin)
        {
            int bsMin = ceilDivision(-(_c - _sumMin + iMin), -_as_neg[i]);
            _bs_neg[i]->removeBelow(bsMin);
            _sumMax = _sumMax - iMax + _as_neg[i]*bsMin;
        }
    }
}