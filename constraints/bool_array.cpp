#include <limits>
#include <constraints/bool_array.hpp>

array_bool_and_reif::array_bool_and_reif(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars) :
    Constraint(cp),
    _as(),
    _r(bool_vars[fzConstraint.vars.back()])
{
    for(size_t i = 0; i < fzConstraint.vars.size() - 1; i += 1)
    {
        _as.push_back(bool_vars[fzConstraint.vars[i]]);
    }
}

void array_bool_and_reif::post()
{
    for(auto x : _as)
    {
        x->propagateOnBind(this);
    }
    _r->propagateOnBind(this);
}

void array_bool_and_reif::propagate()
{
    //Semantic: as1 /\ ... /\ asn <-> r
    bool asSatisfied = true;
    int notBoundCount = 0;
    var<bool>::Ptr asNotBound = nullptr;
    for(auto x : _as)
    {
        if(not x->isBound())
        {
            notBoundCount += 1;
            asNotBound = x;
        }
        else if (x->isFalse())
        {
            asSatisfied = false;
            break;
        }
    }

    //Propagation: as1 /\ ... /\ asn -> r
    if (not asSatisfied)
    {
        _r->assign(false);
    }
    else if (notBoundCount == 0)
    {
        _r->assign(true);
    }

    //Propagation: as1 /\ ... /\ asn <- r
    if(_r->isBound())
    {
        if (_r->isTrue())
        {
            for(auto x : _as)
            {
                x->assign(true);
            }
        }
        else if (notBoundCount == 1)
        {
            asNotBound->assign(false);
        }
    }
}

array_bool_element::array_bool_element(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars) :
    Constraint(cp),
    _b(int_vars[fzConstraint.vars[0]]),
    _as(),
    _c(bool_vars[fzConstraint.vars[1]])
{
    _as.push_back(0); // Index from 1
    for(size_t i = 0; i < fzConstraint.consts.size(); i += 1)
    {
        _as.push_back(fzConstraint.consts[i]);
    }
}

void array_bool_element::post()
{
    _b->updateBounds(1,_as.size());
    _b->propagateOnDomainChange(this);
    _c->propagateOnBind(this);
    propagate();
}

void array_bool_element::propagate()
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
        if (_b->contains(bVal) and not _c->contains(_as[bVal]))
        {
            _b->remove(bVal);
        }
    }
}

array_bool_or_reif::array_bool_or_reif(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars) :
    Constraint(cp),
    _as(),
    _r(bool_vars[fzConstraint.vars.back()])
{
    for(size_t i = 0; i < fzConstraint.vars.size() - 1; i += 1)
    {
        _as.push_back(bool_vars[fzConstraint.vars[i]]);
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

void array_bool_or_reif::propagate()
{
    //Semantic: as1 \/ ... \/ asn <-> r
    bool asSatisfied = false;
    int notBoundCount = 0;
    var<bool>::Ptr asNotBound = nullptr;
    for (auto x : _as)
    {
        if(not x->isBound())
        {
            notBoundCount += 1;
            asNotBound = x;
        }
        else if (x->isTrue())
        {
            asSatisfied = true;
            break;
        }
    }

    //Propagation: as1 \/ ... \/ asn -> r
    if (asSatisfied)
    {
        _r->assign(true);
    }
    else if (notBoundCount == 0)
    {
        _r->assign(false);
    }

    //Propagation: as1 \/ ... \/ asn <- r
    if(_r->isBound())
    {
        if(_r->isFalse())
        {
            for (auto x : _as)
            {
                x->assign(false);
            }
        }
        else if (notBoundCount == 1)
        {
            asNotBound->assign(true);
        }
    }
}

array_bool_xor::array_bool_xor(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars) :
    Constraint(cp),
    _as()
{
    for(size_t i = 0; i < fzConstraint.vars.size(); i += 1)
    {
        _as.push_back(bool_vars[fzConstraint.vars[i]]);
    }
}

void array_bool_xor::post()
{
    for(auto v : _as)
    {
        v->propagateOnBind(this);
    }
    propagate();
}

void array_bool_xor::propagate()
{
    //Semantic: as1 + ... + asn (The number of true variables is odd)
    int trueCount = 0;
    int notBoundCount = 0;
    var<bool>::Ptr asNotBound = nullptr;
    for (auto x : _as)
    {
        if(not x->isBound())
        {
            notBoundCount += 1;
            asNotBound = x;
        }
        else if (x->isTrue())
        {
            trueCount += 1;
        }
    }

    //Propagation: as1 + ... + asn
    if(notBoundCount == 1)
    {
        asNotBound->assign(trueCount % 2 == 0);
    }
}

array_var_bool_element::array_var_bool_element(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars) :
    Constraint(cp),
    _b(int_vars[fzConstraint.vars.front()]),
    _as(),
    _c(bool_vars[fzConstraint.vars.back()])
{
    _as.push_back(nullptr); // Index from 1
    for(size_t i = 1; i < fzConstraint.vars.size() - 1; i += 1)
    {
        _as.push_back(bool_vars[fzConstraint.vars[i]]);
    }
}

void array_var_bool_element::post()
{
    _b->updateBounds(1,_as.size());
    for(size_t i = 1; i < _as.size(); i += 1)
    {
        _as[i]->propagateOnBind(this);
    }
    _b->propagateOnBind(this);
    propagate();
}

void array_var_bool_element::propagate()
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
    if(_b->isBound())
    {
        _as[bMin]->updateBounds(cMin, cMax);
    }
    else if(_c->isBound())
    {
        int cVal = _c->min();
        for (int bVal = bMin; bVal <= bMax; bVal += 1)
        {
            if (_b->contains(bVal) and (not _as[bVal]->contains(cVal)))
            {
                _b->remove(bVal);
            }
        }
    }
}