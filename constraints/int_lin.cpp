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

void int_lin::calSumMinMax(int_lin* il)
{
    auto& _as_pos = il->_as_pos;
    auto& _as_neg = il->_as_neg;
    auto& _bs_pos = il->_bs_pos;
    auto& _bs_neg = il->_bs_neg;
    auto& _sumMin = il->_sumMin;
    auto& _sumMax = il->_sumMax;
    auto& _posNotBoundCount = il->_posNotBoundCount;
    auto& _negNotBoundCount = il->_negNotBoundCount;
    auto& _posNotBoundIdx = il->_posNotBoundIdx;
    auto& _negNotBoundIdx = il->_negNotBoundIdx;

    _sumMin = 0;
    _sumMax = 0;
    _posNotBoundCount = 0;
    _negNotBoundCount = 0;
    _posNotBoundIdx = 0;
    _negNotBoundIdx = 0;
    for (size_t i = 0; i < _as_pos.size(); i += 1)
    {
        auto& asi = _as_pos[i];
        auto& bsi = _bs_pos[i];
        int bsMin = bsi->min();
        int bsMax = bsi->max();

        _sumMin += asi * bsMin;
        _sumMax += asi * bsMax;

        if(bsMin != bsMax)
        {
            _posNotBoundCount += 1;
            _posNotBoundIdx = i;
        }
    }
    for (size_t i = 0; i < _as_neg.size(); i += 1)
    {
        auto& asi = _as_neg[i];
        auto& bsi = _bs_neg[i];
        int bsMin = bsi->min();
        int bsMax = bsi->max();

        _sumMin += asi * bsMax;
        _sumMax += asi * bsMin;

        if(bsMin != bsMax)
        {
            _negNotBoundCount += 1;
            _negNotBoundIdx = i;
        }
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
    calSumMinMax(this);

    propagate(this);

    if (_c < _sumMin or _sumMax < _c)
    {
        failNow();
    }
}

void int_lin_eq::propagate(int_lin* il)
{
    //Semantic: as1*bs1 + ... + asn*bsn = c

    //Propagation: as1*bs1 + ... + asn*bsn <- c
    int_lin_ge::propagate(il, il->_c);
    int_lin_le::propagate(il);
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
    calSumMinMax(this);

    //Propagation: as1*bs1 + ... + asn*bsn = c -> r
    if(_sumMin == _sumMax)
    {
        _r->assign(_sumMin == _c);
        setActive(false);
    }
    else if(_c < _sumMin or _sumMax < _c)
    {
        _r->assign(false);
        setActive(false);
    }
    else if (_r->isBound()) //Propagation: as1*bs1 + ... + asn*bsn = c <- r
    {
        if(_r->isTrue())
        {
            int_lin_eq::propagate(this);
        }
        else
        {
            int_lin_ne::propagate(this);
        }
    }
}

void int_lin_ge::propagate(int_lin* il, int c)
{
    //Semantic: as1*bs1 + ... + asn*bsn >= c
    auto& _as_pos = il->_as_pos;
    auto& _as_neg = il->_as_neg;
    auto& _bs_pos = il->_bs_pos;
    auto& _bs_neg = il->_bs_neg;
    auto& _c = c;
    auto& _sumMin = il->_sumMin;
    auto& _sumMax = il->_sumMax;

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
    calSumMinMax(this);

    propagate(this);

    if(_sumMax <= _c)
    {
        setActive(false);
    }
    else if(_c < _sumMin)
    {
        failNow();
    }
}

void int_lin_le::propagate(int_lin* il)
{
    //Semantic: as1*bs1 + ... + asn*bsn <= c
    auto& _as_pos = il->_as_pos;
    auto& _as_neg = il->_as_neg;
    auto& _bs_pos = il->_bs_pos;
    auto& _bs_neg = il->_bs_neg;
    auto& _c = il->_c;
    auto& _sumMin = il->_sumMin;
    auto& _sumMax = il->_sumMax;

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
    calSumMinMax(this);

    //Propagation: as1*bs1 + ... + asn*bsn <= c -> r
    if(_sumMax <= _c)
    {
        _r->assign(true);
        setActive(false);
    }
    else if(_c < _sumMin)
    {
        _r->assign(false);
        setActive(false);
    }
    else if (_r->isBound())  //Bound propagation: as1*bs1 + ... + asn*bsn <= c <- r
    {
        if (_r->isTrue())
        {
            int_lin_le::propagate(this);
        }
        else
        {
            int_lin_ge::propagate(this, _c + 1);
        }
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
    calSumMinMax(this);

    propagate(this);

    if(_c < _sumMin or _sumMax < _c)
    {
        setActive(false);
    }
    else if (_sumMin == _sumMax and _sumMin == _c)
    {
        failNow();
    }
}

void int_lin_ne::propagate(int_lin* il)
{
    //Semantic: as1*bs1 + ... + asn*bsn != c
    auto& _as_pos = il->_as_pos;
    auto& _as_neg = il->_as_neg;
    auto& _bs_pos = il->_bs_pos;
    auto& _bs_neg = il->_bs_neg;
    auto& _c = il->_c;
    auto& _sumMin = il->_sumMin;
    auto& _posNotBoundCount = il->_posNotBoundCount;
    auto& _negNotBoundCount = il->_negNotBoundCount;
    auto& _posNotBoundIdx = il->_posNotBoundIdx;
    auto& _negNotBoundIdx = il->_posNotBoundIdx;

    //Propagation: as1*bs1 + ... + asn*bsn <- c
    if (_posNotBoundCount + _negNotBoundCount == 1)
    {
        auto& asNotBound = _posNotBoundCount == 1 ? _as_pos[_posNotBoundIdx] : _as_neg[_negNotBoundIdx];
        auto& bsNotBound = _posNotBoundCount == 1 ? _bs_pos[_posNotBoundIdx] : _bs_neg[_negNotBoundIdx];
        if((_c - _sumMin) % asNotBound == 0)
        {
            bsNotBound->remove((_c - _sumMin) / asNotBound);
        }
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
    calSumMinMax(this);

    //Propagation: as1*bs1 + ... + asn*bsn <= c -> r
    if(_sumMin == _sumMax)
    {
        _r->assign(_sumMin != _c);
        setActive(false);
    }
    else if(_c < _sumMin or _sumMax < _c)
    {
        _r->assign(true);
        setActive(false);
    }
    else if (_r->isBound()) //Propagation: as1*bs1 + ... + asn*bsn <= c <- r
    {
        if(_r->isTrue())
        {
            int_lin_ne::propagate(this);
        }
        else
        {
            int_lin_eq::propagate(this);
        }
    }
}