#include "constraint_flatzinc_bool.hpp"

array_bool_or_reif::array_bool_or_reif(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const& vars, std::vector<int> const& consts) :
    Constraint(cp),
    _as(),
    _r(boolVars->at(vars.back()))
{
    for(size_t i = 0; i < vars.size() - 1; i += 1)
    {
        _as.push_back(boolVars->at(vars[i]));
    }
}

void array_bool_or_reif::propagate()
{
    bool asSatisfied = false;
    int notBoundCount = 0;
    int notBoundIndex = 0;
    for (size_t i = 0; i < _as.size(); i += 1)
    {
        if(not _as[i]->isBound())
        {
            notBoundCount += 1;
            notBoundIndex = i;
        }
        else if (_as[i]->isTrue())
        {
            asSatisfied = true;
            break;
        }
    }

    // as -> r
    if (asSatisfied)
    {
        _r->assign(true);
    }
    else if (notBoundCount == 0)
    {
        _r->assign(false);
    }

    // r -> as
    if(_r->isBound() and notBoundCount == 1)
    {
        if (not asSatisfied)
        {
            _as[notBoundIndex]->assign(_r->isTrue());
        }
    }
}

void array_bool_or_reif::post()
{
    for(auto v : _as)
    {
        v->propagateOnBind(this);
    }
    _r->propagateOnBind(this);
}

bool_clause::bool_clause(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const& vars, std::vector<int> const& consts):
    Constraint(cp),
    _as(),
    _bs()
{
    for(int i = 1; i < consts[0]; i += 1)
    {
        _as.push_back(boolVars->at(vars[i]));
    }
    for(int i = consts[0]; i < consts[1]; i += 1)
    {
        _bs.push_back(boolVars->at(vars[i]));
    }
}

void bool_clause::propagate()
{
    bool asSatisfied = false;
    int asNotBoundCount = 0;
    int asNotBoundIndex = 0;
    for(size_t i = 0; i < _as.size(); i += 1)
    {
        if(not _as[i]->isBound())
        {
            asNotBoundCount += 1;
            asNotBoundIndex = i;
        }
        else if (_as[i]->isTrue())
        {
            asSatisfied = true;
            break;
        }
    }

    if(not asSatisfied)
    {
        bool bsSatisfied = false;
        int bsNotBoundCount = 0;
        int bsNotBoundIndex = 0;
        for(size_t i = 0; i < _bs.size(); i += 1)
        {
            if(not _bs[i]->isBound())
            {
                bsNotBoundCount += 1;
                bsNotBoundIndex = i;
            }
            else if (_bs[i]->isFalse())
            {
                bsSatisfied = true;
                break;
            }
        }

        if(not bsSatisfied)
        {
            if (asNotBoundCount == 0 and bsNotBoundCount == 0)
            {
                failNow();
            }
            else if (asNotBoundCount == 1 and bsNotBoundCount == 0)
            {
                _as[asNotBoundIndex]->assign(true);
            }
            else if (asNotBoundCount == 0 and bsNotBoundCount == 1)
            {
                _bs[bsNotBoundIndex]->assign(false);
            }
        }
    }
}

void bool_clause::post()
{
    for(auto v : _as)
    {
        v->propagateOnBind(this);
    }
    for(auto v : _bs)
    {
        v->propagateOnBind(this);
    }
}
