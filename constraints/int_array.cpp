#include <algorithm>
#include <limits>
#include <constraints/int_array.hpp>


array_int_element::array_int_element(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars) :
    Constraint(cp),
    _b(int_vars[fzConstraint.vars[0]]),
    _as(),
    _c(int_vars[fzConstraint.vars[1]])
{
    _as.push_back(0); // Index from 1
    for(size_t i = 0; i < fzConstraint.consts.size(); i += 1)
    {
        _as.push_back(fzConstraint.consts[i]);
    }
}

void array_int_element::post()
{
    _b->updateBounds(1,_as.size());
    _b->propagateOnDomainChange(this);
    _c->propagateOnBoundChange(this);
    propagate();
}

void array_int_element::propagate()
{
    //Semantic: as[b] = c
    int bMin = _b->min();
    int bMax = _b->max();
    int cMin = std::numeric_limits<int>::max();
    int cMax = std::numeric_limits<int>::min();

    //Propagation: as[b] -> c
    for (int bVal = bMin; bVal <= bMax; bVal += 1)
    {
        if(_b->contains(bVal))
        {
            cMin = std::min(cMin, _as[bVal]);
            cMax = std::max(cMax, _as[bVal]);
        }
    }
    _c->updateBounds(cMin, cMax);

    //Propagation: as[b] <- c
    for (int bVal = bMin; bVal <= bMax; bVal += 1)
    {
        if (_b->contains(bVal) and (not _c->contains(_as[bVal])))
        {
            _b->remove(bVal);
        }
    }
}

array_int_maximum::array_int_maximum(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars) :
    Constraint(cp),
    _m(int_vars[fzConstraint.vars[0]]),
    _x()
{
    for(size_t i = 1; i < fzConstraint.vars.size(); i += 1)
    {
        _x.push_back(int_vars[fzConstraint.vars[i]]);
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

void array_int_maximum::propagate()
{
    //Semantic: max(x1,...,xn) = m
    int mMin = std::numeric_limits<int>::max();
    int mMax = std::numeric_limits<int>::min();

    //Propagation: max(x1,...,xn) -> m
    for (auto x : _x)
    {
        mMin = std::min(mMin, x->max());
        mMax = std::max(mMax, x->max());
    }
    _m->updateBounds(mMin, mMax);

    //Propagation: max(x1,...,xn) <- m
    mMax = _m->max();
    for (auto x : _x)
    {
        x->removeAbove(mMax);
    }
}

array_int_minimum::array_int_minimum(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars) :
    Constraint(cp),
    _m(int_vars[fzConstraint.vars[0]]),
    _x()
{
    for(size_t i = 1; i < fzConstraint.vars.size(); i += 1)
    {
        _x.push_back(int_vars[fzConstraint.vars[i]]);
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

void array_int_minimum::propagate()
{
    //Semantic: min(x1,...,xn) = m
    int mMin = std::numeric_limits<int>::max();
    int mMax = std::numeric_limits<int>::min();

    //Propagation: min(x1,...,xn) -> m
    for(auto x : _x)
    {
        mMin = std::min(mMin, x->min());
        mMax = std::max(mMax, x->min());
    }
    _m->updateBounds(mMin, mMax);

    //Propagation: min(x1,...,xn) <- m
    mMin = _m->min();
    for (auto x : _x)
    {
        x->removeBelow(mMin);
    }
}

array_var_int_element::array_var_int_element(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars) :
    Constraint(cp),
    _b(int_vars[fzConstraint.vars[0]]),
    _as(),
    _c(int_vars[fzConstraint.vars.back()])
{
    _as.push_back(nullptr); // Index from 1
    for(size_t i = 1; i < fzConstraint.vars.size() - 1; i += 1)
    {
        _as.push_back(int_vars[fzConstraint.vars[i]]);
    }
}

void array_var_int_element::post()
{
    _b->updateBounds(1,_as.size());
    for(size_t i = 1; i < _as.size(); i += 1) // Index from 1
    {
        _as[i]->propagateOnBoundChange(this);
    }
    _b->propagateOnDomainChange(this);
    _c->propagateOnBoundChange(this);
    propagate();
}

void array_var_int_element::propagate()
{
    //Semantic: as[b] = c
    int bMin = _b->min();
    int bMax = _b->max();
    int cMin = std::numeric_limits<int>::max();
    int cMax = std::numeric_limits<int>::min();

    //Propagation: as[b] -> c
    for (int bVal = bMin; bVal <= bMax; bVal += 1)
    {
        if(_b->contains(bVal))
        {
            cMin = std::min(cMin, _as[bVal]->min());
            cMax = std::max(cMax, _as[bVal]->max());
        }
    }
    _c->updateBounds(cMin, cMax);

    //Propagation: as[b] <- c
    cMin = _c->min();
    cMax = _c->max();
    if(_b->isBound())
    {
        _as[bMin]->updateBounds(cMin, cMax);
    }
    else
    {
        for (int bVal = bMin; bVal <= bMax; bVal += 1)
        {
            if (_b->contains(bVal) and (_as[bVal]->max() < cMin or cMax < _as[bVal]->min()))
            {
                _b->remove(bVal);
            }
        }
    }
}