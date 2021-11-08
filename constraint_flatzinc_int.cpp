#include <algorithm>
#include <climits>
#include "utilities.hpp"
#include "constraint_flatzinc_int.hpp"

array_int_element::array_int_element(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const & vars, std::vector<int> const & consts) :
    Constraint(cp),
    _b(intVars->at(vars.front())),
    _as(consts),
    _c(intVars->at(vars.back()))
{}

void array_int_element::propagate()
{
    // b -> c
    int cMinValue = INT_MAX;
    int cMaxValue = INT_MIN;
    for(int i = _b->min(); i <= _b->max(); i += 1)
    {
        cMinValue = std::min(cMinValue, _as[i]);
        cMaxValue = std::max(cMaxValue, _as[i]);
    }
    _c->updateBounds(cMinValue, cMaxValue);

    // c -> b
    int bMinValue = _b->min();
    int bMaxValue = _b->max();
    int i = bMinValue;
    while(i <= bMaxValue and (_as[i] < cMinValue or cMaxValue < _as[i]))
    {
        i += 1;
        bMinValue += 1;
    }
    i = bMaxValue;
    while(i >= bMinValue and (_as[i] < cMinValue or cMaxValue < _as[i]))
    {
        i -= 1;
        bMaxValue -= 1;
    }
    _b->updateBounds(bMinValue, bMaxValue);
}

void array_int_element::post()
{
    _b->propagateOnBoundChange(this);
    _c->propagateOnBoundChange(this);
}

array_var_int_element::array_var_int_element(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const& vars, std::vector<int> const& consts) :
    Constraint(cp),
    _b(intVars->at(vars.front())),
    _as(),
    _c(intVars->at(vars.back()))
{}

void array_var_int_element::propagate()
{
    // b -> c
    int cMinValue = INT_MAX;
    int cMaxValue = INT_MIN;
    for(int i = _b->min(); i <= _b->max(); i += 1)
    {
        cMinValue = std::min(cMinValue, _as[i]->min());
        cMaxValue = std::max(cMaxValue, _as[i]->max());
    }
    _c->updateBounds(cMinValue, cMaxValue);

    // c -> b
    int bMinValue = _b->min();
    int bMaxValue = _b->max();
    int i = bMinValue;
    while(i <= bMaxValue and (_as[i]->max() < cMinValue or cMaxValue < _as[i]->min()))
    {
        i += 1;
        bMinValue += 1;
    }
    i = bMaxValue;
    while(i >= bMinValue and (_as[i]->max() < cMinValue or cMaxValue < _as[i]->min()))
    {
        i -= 1;
        bMaxValue -= 1;
    }
    _b->updateBounds(bMinValue, bMaxValue);
}

void array_var_int_element::post()
{
    _b->propagateOnBoundChange(this);
    for(auto v : _as)
    {
        v->propagateOnBoundChange(this);
    }
    _c->propagateOnBoundChange(this);
}

int_eq_reif::int_eq_reif(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const& vars, std::vector<int> const& consts) :
    Constraint(cp),
    _a(intVars->at(vars[0])),
    _b((intVars->at(vars[1]))),
    _r((boolVars->at(vars[2])))
{}

void int_eq_reif::propagate()
{
   // a,b -> r
   if (_a->min() == _b->min() and _a->max() == _b->max())
   {
       _r->assign(true);
   }
   else if (_a->min() > _b->max() or _a->max() < _b->min())
   {
       _r->assign(false);
   }

   // r -> a,b
   if (_r->isTrue())
   {
       int abMinValue = std::max(_a->min(), _b->min());
       int abMaxValue = std::min(_a->max(), _b->min());
       _a->updateBounds(abMinValue, abMaxValue);
       _b->updateBounds(abMinValue, abMaxValue);
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

void int_lin_eq::propagate()
{
   propagate(_as,_bs, _c);
}

void int_lin_eq::propagate(std::vector<int>& _as, std::vector<var<int>::Ptr>& _bs, int _c)
{
    int sumPosMin = 0;
    int sumPosMax = 0;
    int sumNegMin = 0;
    int sumNegMax = 0;

    for (size_t i = 0; i < _bs.size(); i += 1)
    {
        int bsiMinValue = _bs[i]->min();
        int bsiMaxValue = _bs[i]->max();

        int asiValue = _as[i];

        if (asiValue > 0)
        {
            sumPosMin += asiValue * bsiMinValue;
            sumPosMax += asiValue * bsiMaxValue;
        }
        else
        {
            sumNegMin += -asiValue * bsiMinValue;
            sumNegMax += -asiValue * bsiMaxValue;
        }
    }

    // as, c -> bs
    for (size_t i = 0; i < _bs.size(); i += 1)
    {
        int bsiMinValue = _bs[i]->min();
        int bsiMaxValue = _bs[i]->max();
        int asiValue = _as[i];

        int lowerbound;
        int upperbound;

        if (asiValue > 0)
        {
            int bsiMinAddend = asiValue * bsiMinValue;
            int bsiMaxAddend = asiValue * bsiMaxValue;

            int rhsUpperbound = _c - (sumPosMin - bsiMinAddend) + sumNegMax;
            int rhsLowerbound = _c - (sumPosMax - bsiMaxAddend) + sumNegMin;

            int bsiAlpha = floorDivision(rhsUpperbound, asiValue);
            int bsiGamma = ceilDivision(rhsLowerbound, asiValue);

            lowerbound = bsiGamma;
            upperbound = bsiAlpha;
        }
        else if (asiValue < 0)
        {
            int bsiMinAddend = -asiValue * bsiMinValue;
            int bsiMaxAddend = -asiValue * bsiMaxValue;

            int rhsUpperbound = -_c + sumPosMax - (sumNegMin - bsiMinAddend);
            int rhsLowerbound = -_c + sumPosMin - (sumNegMax - bsiMaxAddend);

            int bsiDelta = floorDivision(rhsUpperbound, -asiValue);
            int bsiBeta = ceilDivision(rhsLowerbound, -asiValue);

            lowerbound = bsiBeta;
            upperbound = bsiDelta;
        }
        else
        {
            lowerbound = bsiMinValue;
            upperbound = bsiMaxValue;
        }

        if (bsiMinValue < lowerbound)
        {
            _bs[i]->removeBelow(lowerbound);

            if (asiValue >= 0)
            {
                sumPosMin -= asiValue * bsiMinValue;
                sumPosMin += asiValue * lowerbound;
            }
            else
            {
                sumNegMin -= -asiValue * bsiMinValue;
                sumNegMin += -asiValue * lowerbound;
            }
        }

        if (upperbound < bsiMaxValue)
        {
            _bs[i]->removeAbove(upperbound);

            if (asiValue >= 0)
            {
                sumPosMax -= asiValue * bsiMaxValue;
                sumPosMax += asiValue * upperbound;
            }
            else
            {
                sumNegMax -= -asiValue * bsiMaxValue;
                sumNegMax += -asiValue * upperbound;
            }
        }
    }
}

void int_lin_eq::post()
{
    for(auto v : _bs)
    {
        v->propagateOnBoundChange(this);
    }
}

int_lin_eq_reif::int_lin_eq_reif(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const& vars, std::vector<int> const& consts) :
    Constraint(cp),
    _as(),
    _bs(),
    _c(consts.back()),
    _r(boolVars->at(vars.back()))
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
    int sumMax = 0;
    int sumMin = 0;
    for(size_t i = 0; i < _bs.size(); i += 1)
    {
        int asiValue = _as[i];

        int bsiMinValue = _bs[i]->min();
        int bsiMaxValue = _bs[i]->max();

        if(asiValue >= 0)
        {
            sumMin += asiValue * bsiMinValue;
            sumMax += asiValue * bsiMaxValue;
        }
        else
        {
            sumMin += asiValue * bsiMaxValue;
            sumMax += asiValue * bsiMinValue;
        }
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
        int_lin_eq::propagate(_as, _bs, _c);
    }
    else if (_r->isFalse())
    {
        int_lin_ne::propagate(_as, _bs, _c);
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
    propagate(_as, _bs, _c);
}

void int_lin_ne::propagate(std::vector<int>& _as, std::vector<var<int>::Ptr>& _bs, int c)
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

    // as, c -> bs
    if(notBoundCount == 1)
    {
        int asNotBounded = _as[notBoundIndex];
        if ((-sum + c) % asNotBounded == 0)
        {
            _bs[notBoundIndex]->remove( (-sum + c) / asNotBounded);
        }
    }
}

void int_lin_ne::post()
{
    for(auto v : _bs)
    {
        v->propagateOnBoundChange(this);
    }
}
