#include <utils.hpp>
#include <constraints/int_lin.hpp>

int_lin::int_lin(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars) :
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
            _bs_pos.push_back(int_vars[fzConstraint.vars[i]]);
        }
        else
        {
            _as_neg.push_back(fzConstraint.consts[i]);
            _bs_neg.push_back(int_vars[fzConstraint.vars[i]]);
        }
    }
}

void int_lin::calSumMinMax(int_lin* intLin, int& min, int& max)
{
    auto& _as_pos = intLin->_as_pos;
    auto& _as_neg = intLin->_as_neg;
    auto& _bs_pos = intLin->_bs_pos;
    auto& _bs_neg = intLin->_bs_neg;

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

void int_lin::post()
{
    for(auto x : _bs_pos)
    {
        x->propagateOnBoundChange(this);
    }

    for(auto x : _bs_neg)
    {
        x->propagateOnBoundChange(this);
    }
}

int_lin_reif::int_lin_reif(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars) :
    int_lin(cp, fzConstraint, int_vars, bool_vars),
    _r(bool_vars[fzConstraint.vars.back()])
{}

void int_lin_reif::post()
{
    int_lin::post();
    _r->propagateOnBind(this);
}

int_lin_eq::int_lin_eq(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars) :
    int_lin(cp, fzConstraint, int_vars, bool_vars)
{}

void int_lin_eq::post()
{
    int_lin::post();
    propagate();
}

void int_lin_eq::propagate()
{
    int sumMin;
    int sumMax;
    calSumMinMax(this, sumMin, sumMax);
    propagate(this, sumMin, sumMax);
}

void int_lin_eq::propagate(int_lin* intLin, int sumMin, int sumMax)
{
    //Semantic: as1*bs1 + ... + asn*bsn = c

    //Propagation: as1*bs1 + ... + asn*bsn <- c
    int_lin_ge::propagate(intLin, intLin->_c, sumMin, sumMax);
    int_lin_le::propagate(intLin, sumMin, sumMax);
}

int_lin_eq_reif::int_lin_eq_reif(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars) :
    int_lin_reif(cp, fzConstraint, int_vars, bool_vars)
{}

void int_lin_eq_reif::post()
{
    int_lin_reif::post();
    propagate();
}

void int_lin_eq_reif::propagate()
{
    //Semantic: as1*bs1 + ... + asn*bsn = c <-> r
    int sumMin;
    int sumMax;
    calSumMinMax(this, sumMin, sumMax);

    //Propagation: as1*bs1 + ... + asn*bsn = c -> r
    if(sumMin == sumMax)
    {
        _r->assign(sumMin == _c);
    }
    else if(_c < sumMin or sumMax < _c)
    {
        _r->assign(false);
    }

    //Propagation: as1*bs1 + ... + asn*bsn = c <- r
    if(_r->isTrue())
    {
        int_lin_eq::propagate(this, sumMin, sumMax);
    }
    else if (_r->isFalse())
    {
        int_lin_ne::propagate(this);
    }
}

void int_lin_ge::propagate(int_lin* intLin, int c, int& sumMin, int& sumMax)
{
    //Semantic: as1*bs1 + ... + asn*bsn >= c
    auto& _as_pos = intLin->_as_pos;
    auto& _as_neg = intLin->_as_neg;
    auto& _bs_pos = intLin->_bs_pos;
    auto& _bs_neg = intLin->_bs_neg;
    auto& _c = c;

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

int_lin_le::int_lin_le(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars) :
    int_lin(cp, fzConstraint, int_vars, bool_vars)
{}

void int_lin_le::post()
{
    int_lin::post();
    propagate();
}

void int_lin_le::propagate()
{
    int sumMin;
    int sumMax;
    calSumMinMax(this, sumMin, sumMax);
    propagate(this, sumMin, sumMax);
}

void int_lin_le::propagate(int_lin* intLin, int& sumMin, int& sumMax)
{
    //Semantic: as1*bs1 + ... + asn*bsn <= c
    auto& _as_pos = intLin->_as_pos;
    auto& _as_neg = intLin->_as_neg;
    auto& _bs_pos = intLin->_bs_pos;
    auto& _bs_neg = intLin->_bs_neg;
    auto& _c = intLin->_c;

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

int_lin_le_reif::int_lin_le_reif(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars) :
    int_lin_reif(cp, fzConstraint, int_vars, bool_vars)
{}

void int_lin_le_reif::post()
{
    int_lin_reif::post();
    propagate();
}

void int_lin_le_reif::propagate()
{
    //Semantic: as1*bs1 + ... + asn*bsn <= c <-> r
    int sumMin;
    int sumMax;
    calSumMinMax(this, sumMin, sumMax);

    //Propagation: as1*bs1 + ... + asn*bsn <= c -> r
    if(sumMin == sumMax)
    {
        _r->assign(sumMin == _c);
    }
    else if(_c < sumMin or sumMax < _c)
    {
        _r->assign(false);
    }

    //Bound propagation: as1*bs1 + ... + asn*bsn <= c <- r
    if(_r->isTrue())
    {
        int_lin_le::propagate(this,  sumMin, sumMax);
    }
    else if (_r->isFalse())
    {
        int_lin_ge::propagate(this, _c + 1, sumMin, sumMax);
    }
}

int_lin_ne::int_lin_ne(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars) :
    int_lin(cp, fzConstraint, int_vars, bool_vars)
{}

void int_lin_ne::post()
{
    for(auto x : _bs_pos)
    {
        x->propagateOnBind(this);
    }
    for(auto x : _bs_neg)
    {
        x->propagateOnBind(this);
    }
    propagate();
}

void int_lin_ne::propagate()
{
    propagate(this);
}

void int_lin_ne::propagate(int_lin* intLin)
{
    //Semantic: as1*bs1 + ... + asn*bsn != c
    auto& _as_pos = intLin->_as_pos;
    auto& _as_neg = intLin->_as_neg;
    auto& _bs_pos = intLin->_bs_pos;
    auto& _bs_neg = intLin->_bs_neg;
    auto& _c = intLin->_c;

    //Propagation: as1*bs1 + ... + asn*bsn <- c
    int asNotBound = 0;
    var<int>::Ptr bsNotBound = nullptr;
    int notBoundCount = 0;
    int sum = 0;
    for (size_t i = 0; i < _bs_pos.size(); i += 1)
    {
        if (not _bs_pos[i]->isBound())
        {
            notBoundCount += 1;
            asNotBound = _as_pos[i];
            bsNotBound = _bs_pos[i];
        }
        else
        {
            sum += _as_pos[i] * _bs_pos[i]->min();
        }
    }
    for (size_t i = 0; i < _bs_neg.size(); i += 1)
    {
        if (not _bs_neg[i]->isBound())
        {
            notBoundCount += 1;
            asNotBound = _as_neg[i];
            bsNotBound = _bs_neg[i];
        }
        else
        {
            sum += _as_neg[i] * _bs_neg[i]->min();
        }
    }

    if(notBoundCount == 0 and sum == _c)
    {
        failNow();
    }
    else if (notBoundCount == 1 and (_c - sum) % asNotBound == 0)
    {
        bsNotBound->remove((_c - sum) / asNotBound);
    }
}

int_lin_ne_reif::int_lin_ne_reif(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars) :
    int_lin_reif(cp, fzConstraint, int_vars, bool_vars)
{}

void int_lin_ne_reif::post()
{
    int_lin_reif::post();
    propagate();
}

void int_lin_ne_reif::propagate()
{
    //Semantic: as1*bs1 + ... + asn*bsn != c <-> r
    int sumMin;
    int sumMax;
    calSumMinMax(this, sumMin, sumMax);

    //Propagation: as1*bs1 + ... + asn*bsn <= c -> r
    if(sumMin == sumMax)
    {
        _r->assign(sumMin != _c);
    }
    else if(_c < sumMin or sumMax < _c)
    {
        _r->assign(true);
    }

    //Propagation: as1*bs1 + ... + asn*bsn <= c <- r
    if(_r->isTrue())
    {
        int_lin_ne::propagate(this);
    }
    else if (_r->isFalse())
    {
        int_lin_eq::propagate(this, sumMin, sumMax);
    }
}