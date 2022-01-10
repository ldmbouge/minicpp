#include <algorithm>
#include <fz_constraints/int_bin.hpp>

int_bin::int_bin(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars) :
    Constraint(cp),
    _a(int_vars[fzConstraint.vars[0]]),
    _b(int_vars[fzConstraint.vars[1]])
{}

void int_bin::post()
{
    _a->propagateOnBoundChange(this);
    _b->propagateOnBoundChange(this);
}

int_bin_reif::int_bin_reif(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars) :
    int_bin(cp, fzConstraint, int_vars, bool_vars),
    _r(bool_vars[fzConstraint.vars[2]])
{}

void int_bin_reif::post()
{
   int_bin::post();
   _r->propagateOnBind(this);
}

int_abs::int_abs(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars) :
    int_bin(cp, fzConstraint, int_vars, bool_vars)
{}

void int_abs::post()
{
    _b->removeBelow(0);
    int_bin::post();
    propagate();
}

void int_abs::propagate()
{
    //Semantic: b = |a|
    int aMin = _a->min();
    int aMax = _a->max();
    int bMin = _b->min();
    int bMax = _b->max();

    //Propagation: b <- |a|
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

    //Propagation: b -> |a|
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

int_eq::int_eq(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars) :
    int_bin(cp, fzConstraint, int_vars, bool_vars)
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
    int aMin = _a->min();
    int aMax = _a->max();
    int bMin = _b->min();
    int bMax = _b->max();
    int min = std::max(aMin, bMin);
    int max = std::min(aMax, bMax);

    //Propagation: a -> b
    _b->updateBounds(min, max);

    //Propagation: b -> a
    _a->updateBounds(min, max);
}

int_eq_reif::int_eq_reif(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars) :
    int_bin_reif(cp, fzConstraint, int_vars, bool_vars)
{}

void int_eq_reif::post()
{
    int_bin_reif::post();
    propagate();
}

void int_eq_reif::propagate()
{
    //Semantic: a = b <-> r
    int aMin = _a->min();
    int aMax = _a->max();
    int bMin = _b->min();
    int bMax = _b->max();

    //Propagation: a = b -> r
    if (aMin == aMax and bMin == bMax and aMin == bMin)
    {
       _r->assign(true);
       setActive(false);
    }
    else if(aMax < bMin or bMax < aMin)
    {
        _r->assign(false);
        setActive(false);
    }
    /*
    else if(aMin == aMax and (not _b->contains(aMin)))
    {
        _r->assign(false);
        setActive(false);
    }
    else if (bMin == bMax and (not _a->contains(bMin)))
    {
        _r->assign(false);
        setActive(false);
    }
     */

    //Propagation: a = b <- r
    if (_r->isBound())
    {
       if (_r->isTrue())
       {
           int_eq::propagate(this, _a, _b);
       }
       else
       {
           int_ne::propagate(this, _a, _b);
       }
    }
}

int_le::int_le(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars) :
    int_bin(cp, fzConstraint, int_vars, bool_vars)
{}

void int_le::post()
{
    int_bin::post();
    propagate();
}

void int_le::propagate()
{
    propagate(this, _a, _b);

    if(_a->max() <= _b->min())
    {
        setActive(false);
    }
}

void int_le::propagate(Constraint* c, var<int>::Ptr _a, var<int>::Ptr _b)
{
    //Semantic: a <= b
    int aMin = _a->min();
    int bMax = _b->max();

    //Propagation: b -> a
    _a->removeAbove(bMax);

    //Propagation: a -> b
    _b->removeBelow(aMin);
}

int_le_reif::int_le_reif(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars) :
    int_bin_reif(cp, fzConstraint, int_vars, bool_vars)
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

    //Propagation: a <= b -> r
    if (aMax <= bMin)
    {
        _r->assign(true);
        setActive(false);
    }
    else if (bMax < aMin)
    {
        _r->assign(false);
        setActive(false);
    }

    //Propagation: a <= b <- r
    if (_r->isBound())
    {
        if (_r->isTrue())
        {
            int_le::propagate(this, _a, _b);
        }
        else
        {
            int_lt::propagate(this, _b, _a);
        }
    }
}

int_lt::int_lt(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars) :
    int_bin(cp, fzConstraint, int_vars, bool_vars)
{}

void int_lt::post()
{
    int_bin::post();
    propagate();
}

void int_lt::propagate()
{
    propagate(this, _a, _b);

    if(_a->max() < _b->min())
    {
        setActive(false);
    }
}

void int_lt::propagate(Constraint* c, var<int>::Ptr _a, var<int>::Ptr _b)
{
    //Semantic: a < b
    int aMin = _a->min();
    int bMax = _b->max();

    //Propagation: b -> a
    _a->removeAbove(bMax - 1);

    //Propagation: a -> b
    _b->removeBelow(aMin + 1);
}

int_lt_reif::int_lt_reif(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars) :
    int_bin_reif(cp, fzConstraint, int_vars, bool_vars)
{}

void int_lt_reif::post()
{
    int_bin_reif::post();
    propagate();
}

void int_lt_reif::propagate()
{
    //Semantic: a < b <-> r
    int aMin = _a->min();
    int aMax = _a->max();
    int bMin = _b->min();
    int bMax = _b->max();

    //Propagation: a < b -> r
    if (aMax < bMin)
    {
        _r->assign(true);
        setActive(false);
    }
    else if (bMax <= aMin)
    {
        _r->assign(false);
        setActive(false);
    }
    else if (_r->isBound())  //Propagation: a < b <- r
    {
        if (_r->isTrue())
        {
            int_lt::propagate(this, _a, _b);
        }
        else
        {
            int_le::propagate(this, _b, _a);
        }
    }
}

int_ne::int_ne(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars) :
    int_bin(cp, fzConstraint, int_vars, bool_vars)
{}

void int_ne::post()
{
    int_bin::post();
    propagate();
}

void int_ne::propagate()
{
    propagate(this, _a, _b);

    int aMin = _a->min();
    int aMax = _a->max();
    int bMin = _b->min();
    int bMax = _b->max();
    if(aMax < bMin or bMax < aMin)
    {
        setActive(false);
    }
    else if(aMin == aMax and (not _b->contains(aMin)))
    {
        setActive(false);
    }
    else if (bMin == bMax and (not _a->contains(bMin)))
    {
        setActive(false);
    }
}

void int_ne::propagate(Constraint *c, var<int>::Ptr _a, var<int>::Ptr _b)
{
    //Semantic: a != b
    int aMin = _a->min();
    int aMax = _a->max();
    int bMin = _b->min();
    int bMax = _b->max();

    //Propagation: a -> b
    if(aMin == aMax)
    {
        _b->remove(aMin);
    }

    //Propagation: a <- b
    if (bMin == bMax)
    {
        _a->remove(bMin);
    }
}

int_ne_reif::int_ne_reif(CPSolver::Ptr cp, FlatZinc::Constraint& fzConstraint, std::vector<var<int>::Ptr>& int_vars, std::vector<var<bool>::Ptr>& bool_vars) :
    int_bin_reif(cp, fzConstraint, int_vars, bool_vars)
{}

void int_ne_reif::post()
{
    int_bin_reif::post();
    propagate();
}

void int_ne_reif::propagate()
{
    //Semantic: a != b <-> r
    int aMin = _a->min();
    int aMax = _a->max();
    int bMin = _b->min();
    int bMax = _b->max();

    //Propagation: a != b -> r
    if(aMax < bMin or bMax < aMin)
    {
        _r->assign(true);
        setActive(false);
    }
    else if(aMin == aMax and (not _b->contains(aMin)))
    {
        _r->assign(true);
        setActive(false);
    }
    else if (bMin == bMax and (not _a->contains(bMin)))
    {
        _r->assign(true);
        setActive(false);
    }
    else if (aMin == aMax and bMin == bMax and aMin == bMin)
    {
        _r->assign(false);
        setActive(false);
    }
    else if (_r->isBound()) //Propagation: a != b <- r
    {
        if (_r->isTrue())
        {
            int_ne::propagate(this, _a, _b);
        }
        else
        {
            int_eq::propagate(this, _a, _b);
        }
    }
}
