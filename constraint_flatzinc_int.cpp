#include <algorithm>
#include <climits>
#include <iostream>
#include "utilities.hpp"
#include "constraint_flatzinc_int.hpp"
#include "constraint.hpp"


int_bin::int_bin(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const& vars, std::vector<int> const& consts) :
    Constraint(cp),
    _a(intVars->at(vars[0])),
    _b((intVars->at(vars[1])))
{}

void int_bin::post()
{
    _a->propagateOnBoundChange(this);
    _b->propagateOnBoundChange(this);
}

int_bin_reif::int_bin_reif(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const& vars, std::vector<int> const& consts) :
    int_bin(cp, intVars, boolVars, vars, consts),
    _r((boolVars->at(vars[2])))
{}

void int_bin_reif::post()
{
   int_bin::post();
    _r->propagateOnBind(this);
}

int_lin::int_lin(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const& vars, std::vector<int> const& consts) :
    Constraint(cp),
    _as(),
    _bs(),
    _c(consts[consts.size() - 3]),
    _pos(consts[consts.size() - 2]),
    _neg(consts[consts.size() - 1])
{
    for(size_t i = 0; i < consts.size() - 3; i += 1)
    {
        _as.push_back(consts[i]);
    }

    for(size_t i = 0; i < vars.size(); i += 1)
    {
        _bs.push_back(intVars->at(vars[i]));
    }
}


void int_lin::calSumMinMax(std::vector<int>& _as, std::vector<var<int>::Ptr>& _bs, int _pos, int _neg, int& sumMin, int& sumMax)
{
    for (int i = 0; i < _pos; i += 1)
    {
        sumMin += _as[i] * _bs[i]->min();
        sumMax += _as[i] * _bs[i]->max();
    }
    for (int i = _pos; i < _pos + _neg; i += 1)
    {
        sumMin += _as[i] * _bs[i]->max();
        sumMax += _as[i] * _bs[i]->min();
    }
}
void int_lin::post()
{
    for(auto v : _bs)
    {
        v->propagateOnBoundChange(this);
    }
}

int_lin_reif::int_lin_reif(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const& vars, std::vector<int> const& consts) :
    int_lin(cp, intVars, boolVars, vars, consts),
    _r(boolVars->at(vars.back()))
{}

void int_lin_reif::post()
{
    int_lin::post();
    _r->propagateOnBind(this);
}

array_int_element::array_int_element(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const & vars, std::vector<int> const & consts) :
    Constraint(cp),
    _b(intVars->at(vars.front())),
    _as(),
    _c(intVars->at(vars.back()))
{
    _as.push_back(0); // Index from 1
    for(size_t i = 0; i < consts.size(); i += 1)
    {
        _as.push_back(consts[i]);
    }
}

void array_int_element::propagate()
{
    int bMin = _b->min();
    int bMax = _b->max();
    int cMin = INT_MAX;
    int cMax = INT_MIN;

    // b -> c
    for (int bVal = bMin; bVal <= bMax; bVal += 1)
    {
        if(_b->contains(bVal))
        {
            cMin = std::min(cMin, _as[bVal]);
            cMax = std::max(cMax, _as[bVal]);
        }
    }
    _c->updateBounds(cMin, cMax);

    // c -> b
    for (int bVal = bMin; bVal <= bMax; bVal += 1)
    {
        if (_b->contains(bVal) and not _c->contains(_as[bVal]))
        {
            _b->remove(bVal);
        }
    }
}

void array_int_element::post()
{
    _b->updateBounds(1,_as.size());
    _b->propagateOnDomainChange(this);
    _c->propagateOnBoundChange(this);
    propagate();
}

array_int_maximum::array_int_maximum(CPSolver::Ptr cp, std::vector<var<int>::Ptr> *intVars, std::vector<var<bool>::Ptr> *boolVars, const std::vector<int> &vars, const std::vector<int> &consts) :
    Constraint(cp),
    _m(intVars->at(vars.front())),
    _x()
{
    for(size_t i = 1; i < vars.size(); i += 1)
    {
        _x.push_back(intVars->at(vars[i]));
    }
}

void array_int_maximum::propagate()
{
    int mMin = INT_MAX;
    int mMax = INT_MIN;

    // x -> m
    for (auto xi : _x)
    {
        mMin = std::min(mMin, xi->max());
        mMax = std::max(mMax, xi->max());
    }
    _m->updateBounds(mMin, mMax);

    // m -> x
    mMax = _m->max();
    for (auto x : _x)
    {
        x->removeAbove(mMax);
    }
}

void array_int_maximum::post()
{
    _m->propagateOnBoundChange(this);
    for (auto x : _x)
    {
        x->propagateOnBoundChange(this);
    }
    propagate();
}

array_int_minimum::array_int_minimum(CPSolver::Ptr cp, std::vector<var<int>::Ptr> *intVars, std::vector<var<bool>::Ptr> *boolVars, const std::vector<int> &vars, const std::vector<int> &consts) :
        Constraint(cp),
        _m(intVars->at(vars.front())),
        _x()
{
    for(size_t i = 1; i < vars.size(); i += 1)
    {
        _x.push_back(intVars->at(vars[i]));
    }
}

void array_int_minimum::propagate()
{
    int mMin = INT_MAX;
    int mMax = INT_MIN;

    // x -> m
    for (size_t i = 0; i < _x.size(); i += 1)
    {
        mMin = std::min(mMin, _x[i]->min());
        mMax = std::max(mMax, _x[i]->min());
    }
    _m->updateBounds(mMin, mMax);

    // m -> x
    mMin = _m->min();
    for (auto x : _x)
    {
        x->removeBelow(mMin);
    }
}

void array_int_minimum::post()
{
    _m->propagateOnBoundChange(this);
    for (auto x : _x)
    {
        x->propagateOnBoundChange(this);
    }
    propagate();
}

array_var_int_element::array_var_int_element(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const& vars, std::vector<int> const& consts) :
    Constraint(cp),
    _b(intVars->at(vars.front())),
    _as(),
    _c(intVars->at(vars.back()))
{
    _as.push_back(nullptr); // Index from 1
    for(size_t i = 1; i < vars.size() - 1; i += 1)
    {
        _as.push_back(intVars->at(vars[i]));
    }
}

void array_var_int_element::propagate()
{
    int bMin = _b->min();
    int bMax = _b->max();
    int cMin = INT_MAX;
    int cMax = INT_MIN;

    // as[b] -> c
    for (int bVal = bMin; bVal <= bMax; bVal += 1)
    {
        if(_b->contains(bVal))
        {
            cMin = std::min(cMin, _as[bVal]->min());
            cMax = std::max(cMax, _as[bVal]->max());
        }
    }
    _c->updateBounds(cMin, cMax);
    cMin = _c->min();
    cMax = _c->max();

    if(_b->isBound())
    {
        // c -> as[b]
        _as[bMin]->updateBounds(cMin, cMax);
    }
    else
    {
        // as[b], c -> b
        for (int bVal = bMin; bVal <= bMax; bVal += 1)
        {
            if (_b->contains(bVal) and (_as[bVal]->max() < cMin or cMax < _as[bVal]->min()))
            {
                _b->remove(bVal);
            }
        }
    }
}

void array_var_int_element::post()
{
    _b->updateBounds(1,_as.size());
    for(size_t i = 1; i < _as.size(); i += 1)
    {
        _as[i]->propagateOnBoundChange(this);
    }
    _b->propagateOnDomainChange(this);
    _c->propagateOnBoundChange(this);
    propagate();
    /*
    auto e1dv = new (_b->getSolver()) Element1DVar(_as,_b,_c);
    _b->getSolver()->post(e1dv);
     */
}

int_abs::int_abs(CPSolver::Ptr cp, std::vector<var<int>::Ptr> *intVars, std::vector<var<bool>::Ptr> *boolVars, const std::vector<int> &vars, const std::vector<int> &consts) :
    Constraint(cp),
    _a(intVars->at(vars[0])),
    _b((intVars->at(vars[1])))
{}

void int_abs::propagate()
{
    //Semantic: b = |a|
    int aMin = _a->min();
    int aMax = _a->max();
    int bMin = _b->min();
    int bMax = _b->max();

    //Bound propagation: a -> b
    if(aMin >= 0)
    {
        bMin = std::max(bMin, aMin);
        bMax = std::min(bMax, aMax);
    }
    else if (aMax <= 0)
    {
        bMin = std::max(bMin, -aMax);
        bMax = std::min(bMax, -aMin);
    }
    else
    {
        bMin = std::max(bMin, 0);
        bMax = std::min(bMax, std::max(-aMin, aMax));
    }
    _b->updateBounds(bMin,bMax);

    // Bound propagation: b -> a
    if(aMin >= 0)
    {
        aMin = std::max(aMin, bMin);
        aMax = std::min(aMax, bMax);
    }
    else if (aMax <= 0)
    {
        aMin = std::max(aMin, -bMax);
        aMax = std::min(aMax, -bMin);
    }
    else
    {
        aMin = std::max(aMin, -bMax);
        aMax = std::min(aMax, bMax);
    }
    _a->updateBounds(aMin,aMax);
}

void int_abs::post()
{
    _a->propagateOnBoundChange(this);
    _b->propagateOnBoundChange(this);
    propagate();
}

int_div::int_div(CPSolver::Ptr cp, std::vector<var<int>::Ptr> *intVars, std::vector<var<bool>::Ptr> *boolVars, const std::vector<int> &vars, const std::vector<int> &consts) :
    Constraint(cp),
    _a(intVars->at(vars[0])),
    _b((intVars->at(vars[1]))),
    _c((intVars->at(vars[2])))
{}

void int_div::propagate()
{
    //Semantic: a / b = c
    int aMin = _a->min();
    int aMax = _a->max();
    int bMin = _b->min();
    int bMax = _b->max();
    int cMin;
    int cMax;
    int boundsMin;
    int boundsMax;
    int bounds[4];

    //Bound propagation: a / b -> c
    boundsMin = INT_MAX;
    boundsMax = INT_MIN;
    bounds[0] = aMin / bMin;
    bounds[1] = aMax / bMin;
    bounds[2] = aMin / bMax;
    bounds[3] = aMax / bMax;
    for(int i  = 0; i < 4; i += 1)
    {
        boundsMin = std::min(boundsMin, bounds[i]);
        boundsMax = std::max(boundsMax, bounds[i]);
    }
    _c->updateBounds(boundsMin, boundsMax);
    cMin = _c->min();
    cMax = _c->max();

    //Bound propagation: c * b -> a
    boundsMin = INT_MAX;
    boundsMax = INT_MIN;
    bounds[0] = cMin * bMin;
    bounds[1] = cMax * bMin;
    bounds[2] = cMin * bMax;
    bounds[3] = cMax * bMax;
    for(int i  = 0; i < 4; i += 1)
    {
        boundsMin = std::min(boundsMin, bounds[i]);
        boundsMax = std::max(boundsMax, bounds[i]);
    }
    _a->updateBounds(boundsMin, boundsMax);
    aMin = _a->min();
    aMax = _a->max();

    //Bound propagation: a / c -> b
    boundsMin = INT_MAX;
    boundsMax = INT_MIN;
    bounds[0] = aMin / cMin;
    bounds[1] = aMax / cMin;
    bounds[2] = aMin / cMax;
    bounds[3] = aMax / cMax;
    for(int i  = 0; i < 4; i += 1)
    {
        boundsMin = std::min(boundsMin, bounds[i]);
        boundsMax = std::max(boundsMax, bounds[i]);
    }
    _b->updateBounds(boundsMin, boundsMax);
}

void int_div::post()
{
    _b->remove(0);
    _a->propagateOnBoundChange(this);
    _b->propagateOnBoundChange(this);
    _c->propagateOnBoundChange(this);
    propagate();
}

int_eq::int_eq(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const& vars, std::vector<int> const& consts) :
    int_bin(cp, intVars, boolVars, vars, consts)
{}

void int_eq::post()
{
    int_bin::post();
    propagate();
}

void int_eq::propagate()
{
   propagate(this, _a, _b);
}

void int_eq::propagate(Constraint* c, var<int>::Ptr _a, var<int>::Ptr _b)
{
    //Semantic: a = b
    int min = std::max(_a->min(), _b->min());
    int max = std::min(_a->max(), _b->max());

    //Bound propagation: a -> b
    _b->updateBounds(min, max);

    //Bound propagation: b -> a
    _a->updateBounds(min, max);
}

int_eq_reif::int_eq_reif(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const& vars, std::vector<int> const& consts) :
    int_bin_reif(cp, intVars, boolVars, vars, consts)
{}

void int_eq_reif::post()
{
    int_bin_reif::post();
    propagate();
}

void int_eq_reif::propagate()
{
    //Semantic: a == b <-> r
    int aMin = _a->min();
    int aMax = _a->max();
    int bMin = _b->min();
    int bMax = _b->max();

   //Bound consistency: a == b -> r
   if (aMin == aMax and bMin == bMax and aMin == bMin)
   {
       _r->assign(true);
   }
   else if (aMax < bMin or bMax < aMin)
   {
       _r->assign(false);
   }

   //Bound consistency: r -> a == b
   if (_r->isTrue())
   {
       int_eq::propagate(this, _a, _b);
   }
   else if (_r->isFalse())
   {
       int_ne::propagate(this, _a, _b);
   }
}

void int_gt::propagate(Constraint *c, var<int>::Ptr _a, var<int>::Ptr _b)
{
    //Semantic: a > b
    int aMax = _a->max();
    int bMin = _b->min();

    //Bound propagation: a -> b
    _b->removeAbove(aMax - 1);

    //Bound propagation: b -> a
    _a->removeBelow(bMin + 1);
}

int_le::int_le(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const& vars, std::vector<int> const& consts) :
    int_bin(cp, intVars, boolVars, vars, consts)
{}

void int_le::post()
{
    int_bin::post();
    propagate();
}

void int_le::propagate()
{
    propagate(this, _a, _b);
}

void int_le::propagate(Constraint* c, var<int>::Ptr _a, var<int>::Ptr _b)
{
    //Semantic: a <= b
    int aMin = _a->min();
    int bMax = _b->max();

    //Bound propagation: b -> a
    _a->removeAbove(bMax);

    //Bound propagation: a -> b
    _b->removeBelow(aMin);
}

int_le_reif::int_le_reif(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const& vars, std::vector<int> const& consts) :
    int_bin_reif(cp, intVars, boolVars, vars, consts)
{}

void int_le_reif::post()
{
    int_bin_reif::post();
    propagate();
}

void int_le_reif::propagate()
{
    //Semantic: a <= b <-> r
    int aMin = _a->min();
    int aMax = _a->max();
    int bMin = _b->min();
    int bMax = _b->max();

    //Bound consistency: a <= b -> r
    if (aMax <= bMin)
    {
        _r->assign(true);
    }
    else if (bMax < aMin)
    {
        _r->assign(false);
    }

    //Bound consistency: a <= b <- r
    if (_r->isTrue())
    {
        int_le::propagate(this, _a, _b);
    }
    else if (_r->isFalse())
    {
        int_gt::propagate(this, _a, _b);
    }
}

int_lin_eq::int_lin_eq(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const& vars, std::vector<int> const& consts) :
    int_lin(cp, intVars, boolVars, vars, consts)
{}

void int_lin_eq::post()
{
    int_lin::post();
    propagate();
}

void int_lin_eq::propagate()
{
    int sumMin = 0;
    int sumMax = 0;
    calSumMinMax(_as,_bs, _pos, _neg, sumMin, sumMax);
    propagate(this, _as,_bs, _c, _pos, _neg, sumMin, sumMax);
}

void int_lin_eq::propagate(Constraint* c, std::vector<int>& _as, std::vector<var<int>::Ptr>& _bs, int _c, int _pos, int _neg, int sumMin, int sumMax)
{
    //Semantic: as1 * bs1 + ... + asn * bsn = c

    //Bounds propagation: as1 * bs1 + ... + asn * bsn >= c
    int_lin_ge::propagate(c, _as,_bs, _c, _pos, _neg, sumMin, sumMax);

    //Bounds propagation: as1 * bs1 + ... + asn * bsn <= c
    int_lin_le::propagate(c, _as,_bs, _c, _pos, _neg, sumMin, sumMax);
}

int_lin_eq_reif::int_lin_eq_reif(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const& vars, std::vector<int> const& consts) :
    int_lin_reif(cp, intVars, boolVars, vars, consts)
{}

void int_lin_eq_reif::post()
{
    int_lin_reif::post();
    propagate();
}

void int_lin_eq_reif::propagate()
{
    int sumMin = 0;
    int sumMax = 0;
    calSumMinMax(_as,_bs, _pos, _neg, sumMin, sumMax);

    // as,bs,c -> r
    if(sumMin == sumMax)
    {
        _r->assign(sumMin == _c);
    }
    else if(_c < sumMin or sumMax < _c)
    {
        _r->assign(false);
    }

    // r -> as,bs,c
    if(_r->isTrue())
    {
        int_lin_eq::propagate(this, _as, _bs, _c, _pos, _neg, sumMin, sumMax);
    }
    else if (_r->isFalse())
    {
        int_lin_ne::propagate(this, _as, _bs, _c);
    }
}

void int_lin_ge::propagate(Constraint *c, std::vector<int> &_as, std::vector<var<int>::Ptr> &_bs, int _c, int _pos, int _neg, int sumMin, int sumMax)
{
    for (int i = 0; i < _pos; i += 1)
    {
        int iMin = _as[i] * _bs[i]->min();
        int iMax = _as[i] * _bs[i]->max();
        if (iMax - iMin > -(_c - sumMax))
        {
            int bsMin = ceilDivision(_c - sumMax + iMax, _as[i]);
            _bs[i]->removeBelow(bsMin);
            sumMin = sumMin - iMin + _as[i] * bsMin;
        }
    }
    for (int i = _pos; i < _pos + _neg; i += 1)
    {
        int iMin = _as[i] * _bs[i]->max();
        int iMax = _as[i] * _bs[i]->min();
        if (iMax - iMin > -(_c - sumMax))
        {
            int bsMax = floorDivision(-(_c - sumMax + iMax), -_as[i]);
            _bs[i]->removeAbove(bsMax);
            sumMin = sumMin - iMin + _as[i] * bsMax;
        }
    }
}

void int_lin_gt::propagate(Constraint *c, std::vector<int> &_as, std::vector<var<int>::Ptr> &_bs, int _c, int _pos, int _neg, int sumMin, int sumMax)
{
    int_lin_ge::propagate(c, _as, _bs, _c + 1, _pos, _neg, sumMin, sumMax);
}

int_lin_le::int_lin_le(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const& vars, std::vector<int> const& consts) :
    int_lin(cp, intVars, boolVars, vars, consts)
{}

void int_lin_le::post()
{
    int_lin::post();
    propagate();
}

void int_lin_le::propagate()
{
    int sumMin = 0;
    int sumMax = 0;
    for (int i = 0; i < _pos; i += 1)
    {
        sumMin += _as[i] * _bs[i]->min();
        sumMax += _as[i] * _bs[i]->max();
    }
    for (int i = _pos; i < _pos + _neg; i += 1)
    {
        sumMin += _as[i] * _bs[i]->max();
        sumMax += _as[i] * _bs[i]->min();
    }

    propagate(this, _as,_bs, _c, _pos, _neg, sumMin, sumMax);
}

void int_lin_le::propagate(Constraint* c, std::vector<int>& _as, std::vector<var<int>::Ptr>& _bs, int _c, int _pos, int _neg, int sumMin, int sumMax)
{
    //Semantic: as1 * bs1 + ... + asn * bsn <= c

    // Bound consistency
    for (int i = 0; i < _pos; i += 1)
    {
        int iMin = _as[i] * _bs[i]->min();
        int iMax = _as[i] * _bs[i]->max();

        if (iMax - iMin > _c - sumMin)
        {
            int bsMax = floorDivision(_c - sumMin + iMin, _as[i]);
            _bs[i]->removeAbove(bsMax);
            sumMax = sumMax - iMax + _as[i] * bsMax;
        }
    }
    for (int i = _pos; i < _pos + _neg; i += 1)
    {
        int iMin = _as[i] * _bs[i]->max();
        int iMax = _as[i] * _bs[i]->min();
        if (iMax - iMin > _c - sumMin)
        {
            int bsMin = ceilDivision(-(_c - sumMin + iMin), -_as[i]);
            _bs[i]->removeBelow(bsMin);
            sumMax = sumMax - iMax + _as[i] * bsMin;
        }
    }
}

int_lin_le_reif::int_lin_le_reif(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const& vars, std::vector<int> const& consts) :
    int_lin_reif(cp, intVars, boolVars, vars, consts)
{}

void int_lin_le_reif::post()
{
    int_lin_reif::post();
    propagate();
}

void int_lin_le_reif::propagate()
{
    //Semantic: as1 * bs1 + ... + asn * bsn <= c <-> r
    int sumMin = 0;
    int sumMax = 0;
    calSumMinMax(_as,_bs, _pos, _neg, sumMin, sumMax);

    //Bound propagation: as1 * bs1 + ... + asn * bsn <= c -> r
    if(sumMin == sumMax)
    {
        _r->assign(sumMin == _c);
    }
    else if(_c < sumMin or sumMax < _c)
    {
        _r->assign(false);
    }

    //Bound propagation: as1 * bs1 + ... + asn * bsn <= c <- r
    if(_r->isTrue())
    {
        int_lin_le::propagate(this, _as, _bs, _c, _pos, _neg, sumMin, sumMax);
    }
    else if (_r->isFalse())
    {
        int_lin_gt::propagate(this, _as, _bs, _c, _pos, _neg, sumMin, sumMax);
    }
}

int_lin_ne::int_lin_ne(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const& vars, std::vector<int> const& consts) :
    int_lin(cp, intVars, boolVars, vars, consts)
{}

void int_lin_ne::post()
{
    for(auto v : _bs)
    {
        v->propagateOnBind(this);
    }
}

void int_lin_ne::propagate()
{
    propagate(this, _as, _bs, _c);
}

void int_lin_ne::propagate(Constraint* c, std::vector<int>& _as, std::vector<var<int>::Ptr>& _bs, int _c)
{
    int notBoundCount = 0;
    int notBoundIndex = 0;
    int sum = 0;
    for (size_t i = 0; i < _bs.size(); i += 1)
    {
        if (not _bs[i]->isBound())
        {
            notBoundCount += 1;
            notBoundIndex = i;
        }
        else
        {
            sum += _as[i] * _bs[i]->min();
        }
    }

    if(notBoundCount == 0 and sum == _c)
    {
        failNow();
    }
    else if (notBoundCount == 1 and (_c - sum) % _as[notBoundIndex] == 0)
    {
        _bs[notBoundIndex]->remove((_c - sum) / _as[notBoundIndex]);
    }
}


int_lin_ne_reif::int_lin_ne_reif(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const& vars, std::vector<int> const& consts) :
    int_lin_reif(cp, intVars, boolVars, vars, consts)
{}

void int_lin_ne_reif::post()
{
    int_lin_reif::post();
    propagate();
}

void int_lin_ne_reif::propagate()
{
    //Semantic: as1 * bs1 + ... + asn * bsn != c <-> r
    int sumMin = 0;
    int sumMax = 0;
    calSumMinMax(_as,_bs, _pos, _neg, sumMin, sumMax);

    //Bound propagation: as1 * bs1 + ... + asn * bsn <= c -> r
    if(sumMin == sumMax)
    {
        _r->assign(sumMin != _c);
    }
    else if(_c < sumMin or sumMax < _c)
    {
        _r->assign(true);
    }

    //Bound propagation: as1 * bs1 + ... + asn * bsn <= c <- r
    if(_r->isTrue())
    {
        int_lin_ne::propagate(this, _as, _bs, _c);
    }
    else if (_r->isFalse())
    {
        int_lin_eq::propagate(this, _as, _bs, _c, _pos, _neg, sumMin, sumMax);
    }
}


void int_ne::propagate(Constraint *c, var<int>::Ptr _a, var<int>::Ptr _b)
{
    //Semantic: a != b
    int aMin = _a->min();
    int aMax = _a->max();
    int bMin = _b->min();
    int bMax = _b->max();

    // Unit propagation: a -> b
    if(aMin == aMax)
    {
        _b->remove(aMin);
    }

    // Unit propagation: b -> a
    if (bMin == bMax)
    {
        _a->remove(bMin);
    }
}

