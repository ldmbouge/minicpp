#include <constraints/bool_misc.hpp>

bool2int::bool2int(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const& vars, std::vector<int> const& consts) :
    Constraint(cp),
    _a(boolVars->at(vars[0])),
    _b(intVars->at(vars[1]))
{}

void bool2int::post()
{
    _b->updateBounds(0,1);
    _a->propagateOnBind(this);
    _b->propagateOnBind(this);
    propagate();
}

void bool2int::propagate()
{
    //Semantic: a = b
    int min = std::max(_a->min(), _b->min());
    int max = std::min(_a->max(), _b->max());

    //Propagation: a -> b
    _b->updateBounds(min, max);

    //Propagation: a <- b
    _a->updateBounds(min, max);
}

bool_clause::bool_clause(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const& vars, std::vector<int> const& consts):
    Constraint(cp),
    _as(),
    _bs()
{
    for(int i = 0; i < consts[0]; i += 1)
    {
        _as.push_back(boolVars->at(vars[i]));
    }
    for(int i = consts[0]; i < consts[0] + consts[1]; i += 1)
    {
        _bs.push_back(boolVars->at(vars[i]));
    }
    assert(_bs.size() > 0);
}

void bool_clause::post()
{
    for(auto x : _as)
    {
        x->propagateOnBind(this);
    }
    for(auto x : _bs)
    {
        x->propagateOnBind(this);
    }
}

void bool_clause::propagate()
{
    //Semantic: (as1 \/ ... \/ asn) \/ (-bs1 \/ ... \/ -bsm)
    bool asSatisfied = false;
    bool bsSatisfied = false;
    int asNotBoundCount = 0;
    int bsNotBoundCount = 0;
    var<bool>::Ptr asNotBound = nullptr;
    var<bool>::Ptr bsNotBound = nullptr;
    for(auto x : _as)
    {
        if(not x->isBound())
        {
            asNotBoundCount += 1;
            asNotBound= x;
        }
        else if (x->isTrue())
        {
            asSatisfied = true;
            break;
        }
    }
    for(auto x : _bs)
    {
        if(not x->isBound())
        {
            bsNotBoundCount += 1;
            bsNotBound = x;
        }
        else if (x->isFalse())
        {
            bsSatisfied = true;
            break;
        }
    }

    //Propagation: (as1 \/ ... \/ asn) \/ (-bs1 \/ ... \/ -bsm)
    if(not asSatisfied and not bsSatisfied)
    {
        if (asNotBoundCount == 0 and bsNotBoundCount == 0)
        {
            failNow();
        }
        else if (asNotBoundCount == 1 and bsNotBoundCount == 0)
        {
            asNotBound->assign(true);
        }
        else if (asNotBoundCount == 0 and bsNotBoundCount == 1)
        {
            bsNotBound->assign(false);
        }
    }
}