#include <algorithm>
#include <climits>
#include <iostream>
#include "utilities.hpp"
#include "constraint_flatzinc_int.hpp"
#include "constraint.hpp"

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

    /*
    auto e1d = new (_b->getSolver()) Element1D(_as,_b,_c);
    _b->getSolver()->post(e1d);
     */
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

int_eq_reif::int_eq_reif(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const& vars, std::vector<int> const& consts) :
    Constraint(cp),
    _a(intVars->at(vars[0])),
    _b((intVars->at(vars[1]))),
    _r((boolVars->at(vars[2])))
{}

void int_eq_reif::propagate()
{
    int aMin = _a->min();
    int aMax = _a->max();
    int bMin = _b->min();
    int bMax = _b->max();

   // a,b -> r
   if (aMin == aMax and bMin == bMax and aMin == bMin)
   {
       _r->assign(true);
   }
   else if (aMax < bMin or bMax < aMin)
   {
       _r->assign(false);
   }

   // r -> a,b
   if (_r->isTrue())
   {
       int abMin = std::max(aMin,bMin);
       int abMax = std::min(aMax,bMax);
       _a->updateBounds(abMin, abMax);
       _b->updateBounds(abMin, abMax);
   }
   else if (_r->isFalse())
   {
       if(_a->isBound())
       {
           _b->remove(aMin);
       }
       else if (_b->isBound())
       {
           _a->remove(bMin);
       }
   }
}
void int_eq_reif::post()
{
    _a->propagateOnBoundChange(this);
    _b->propagateOnBoundChange(this);
    _r->propagateOnBind(this);
}

int_lin_eq::int_lin_eq(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const& vars, std::vector<int> const& consts) :
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

void int_lin_eq::propagate()
{
   propagate(this, _as,_bs, _c, _pos, _neg);
}

void int_lin_eq::propagate(Constraint* c, std::vector<int>& _as, std::vector<var<int>::Ptr>& _bs, int _c, int _pos, int _neg)
{
    int sumMin = 0;
    int sumMax = 0;
    for (int i = 0; i < _pos; i += 1)
    {
        assert(_as[i] > 0);
        sumMin += _as[i] * _bs[i]->min();
        sumMax += _as[i] * _bs[i]->max();
    }
    for (int i = _pos; i < _pos + _neg; i += 1)
    {
        assert(_as[i] < 0);
        sumMin += _as[i] * _bs[i]->max();
        sumMax += _as[i] * _bs[i]->min();
    }

    if (_c < sumMin or sumMax < _c)
    {
        failNow();
    }

    //a, bs, c -> bs
    // LtEq
    for (int i = 0; i < _pos; i += 1)
    {
        int iMin = _as[i] * _bs[i]->min();
        int iMax = _as[i] * _bs[i]->max();

        if (iMax - iMin > _c - sumMin)
        {
            int iBsMax = floorDivision(_c - sumMin + iMin, _as[i]);
            _bs[i]->removeAbove(iBsMax);
            sumMax = sumMax - iMax + _as[i] * iBsMax;
        }
    }
    for (int i = _pos; i < _pos + _neg; i += 1)
    {
        int iMin = _as[i] * _bs[i]->max();
        int iMax = _as[i] * _bs[i]->min();
        if (iMax - iMin > _c - sumMin)
        {
            int iBsMin = ceilDivision(-(_c - sumMin + iMin), -_as[i]);
            _bs[i]->removeBelow(iBsMin);
            sumMax = sumMax - iMax + _as[i] * iBsMin;
        }
    }

    //GtEq
    for (int i = 0; i < _pos; i += 1)
    {
        int iMin = _as[i] * _bs[i]->min();
        int iMax = _as[i] * _bs[i]->max();
        if (iMax - iMin > -(_c - sumMax))
        {
            int iBsMin = ceilDivision(_c - sumMax + iMax, _as[i]);
            _bs[i]->removeBelow(iBsMin);
            sumMin = sumMin - iMin + _as[i] * iBsMin;
        }
    }
    for (int i = _pos; i < _pos + _neg; i += 1)
    {
        int iMin = _as[i] * _bs[i]->max();
        int iMax = _as[i] * _bs[i]->min();
        if (iMax - iMin > -(_c - sumMax))
        {
            int iBsMax = floorDivision(-(_c - sumMax + iMax), -_as[i]);
            _bs[i]->removeAbove(iBsMax);
            sumMin = sumMin - iMin + _as[i] * iBsMax;
        }
    }
}

void int_lin_eq::post()
{
    for(auto v : _bs)
    {
        v->propagateOnBoundChange(this);
    }
    propagate();
}

int_lin_eq_reif::int_lin_eq_reif(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const& vars, std::vector<int> const& consts) :
    Constraint(cp),
    _as(),
    _bs(),
    _c(consts[consts.size() - 3]),
    _r(boolVars->at(vars.back())),
    _pos(consts[consts.size() - 2]),
    _neg(consts[consts.size() - 1])
{
    for(size_t i = 0; i < consts.size() - 1; i += 1)
    {
        _as.push_back(consts[i]);
    }

    for(size_t i = 0; i < vars.size() - 1; i += 1)
    {
        _bs.push_back(intVars->at(vars[i]));
    }
}

void int_lin_eq_reif::propagate()
{

    int sumMin = 0;
    int sumMax = 0;
    for (int i = 0; i < _pos; i += 1)
    {
        assert(_as[i] > 0);
        sumMin += _as[i] * _bs[i]->min();
        sumMax += _as[i] * _bs[i]->max();
    }
    for (int i = _pos; i < _pos + _neg; i += 1)
    {
        assert(_as[i] < 0);
        sumMin += _as[i] * _bs[i]->max();
        sumMax += _as[i] * _bs[i]->min();
    }

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
        int_lin_eq::propagate(this, _as, _bs, _c, _pos, _neg);
    }
    else if (_r->isFalse())
    {
        int_lin_ne::propagate(this, _as, _bs, _c);
    }
}

void int_lin_eq_reif::post()
{
    for(auto v : _bs)
    {
        v->propagateOnBoundChange(this);
    }
    _r->propagateOnBind(this);
}

int_lin_ne::int_lin_ne(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const& vars, std::vector<int> const& consts) :
    Constraint(cp),
    _as(),
    _bs(),
    _c(consts.back())
{
    for(size_t i = 0; i < consts.size() - 1; i += 1)
    {
        _as.push_back(consts[i]);
    }

    for(size_t i = 0; i < vars.size(); i += 1)
    {
        _bs.push_back(intVars->at(vars[i]));
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

void int_lin_ne::post()
{
    for(auto v : _bs)
    {
        v->propagateOnBoundChange(this);
    }
}
