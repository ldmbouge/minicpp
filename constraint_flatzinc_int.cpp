#include <algorithm>
#include <climits>
#include <iostream>
#include "utilities.hpp"
#include "constraint_flatzinc_int.hpp"

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
    for(size_t i = 1; i < _as.size() ; i += 1) //Index from 1
    {
        _as[i]->propagateOnBoundChange(this);
    }
    _c->propagateOnBoundChange(this);
    propagate();
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
   if (_a->isBound() and _b->isBound() and _a->min() == _b->min())
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
       int abMaxValue = std::min(_a->max(), _b->max());
       _a->updateBounds(abMinValue, abMaxValue);
       _b->updateBounds(abMinValue, abMaxValue);
   }
   else if (_r->isFalse())
   {
       if(_a->isBound())
       {
           _b->remove(_a->min());
       }
       else if (_b->isBound())
       {
           _a->remove(_b->min());
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

    /*
    int sumMin = 0;
    int sumMax = 0;
    for(size_t i = 0; i < _bs.size(); i += 1)
    {
        int ithBoundValue1 = _as[i] * _bs[i]->min();
        int ithBoundValue2 = _as[i] * _bs[i]->max();

        int ithLowerBound = std::min(ithBoundValue1, ithBoundValue2);
        int ithUpperBound = std::max(ithBoundValue1, ithBoundValue2);

        sumMin += ithLowerBound;
        sumMax += ithUpperBound;
    }

    // as, c -> bs
    for (size_t i = 0; i < _bs.size(); i += 1)
    {
        int ithBoundValue1 = _as[i] * _bs[i]->min();
        int ithBoundValue2 = _as[i] * _bs[i]->max();

        int ithLowerBound = std::min(ithBoundValue1, ithBoundValue2);
        int ithUpperBound = std::max(ithBoundValue1, ithBoundValue2);

        int tmpLowerBound = ithLowerBound;
        int tmpUpperBound = ithUpperBound;

        if (_as[i] > 0)
        {
            int tmpLowerBound1 = ceilDivision(_c - (sumMin - ithLowerBound), _as[i]);
            int tmpLowerBound2 = ceilDivision(_c - (sumMax - ithUpperBound), _as[i]);
            tmpLowerBound = std::max(tmpLowerBound1, tmpLowerBound2);

            int tmpUpperBound1 = floorDivision(_c - (sumMin - ithLowerBound), _as[i]);
            int tmpUpperBound2 = floorDivision(_c - (sumMax - ithUpperBound), _as[i]);
            tmpUpperBound = std::min(tmpUpperBound1, tmpUpperBound2);
        }
        else
        {
            int tmpUpperBound1 = ceilDivision(_c - (sumMin - ithLowerBound), _as[i]);
            int tmpUpperBound2 = ceilDivision(_c - (sumMax - ithUpperBound), _as[i]);
            tmpUpperBound = std::min(tmpUpperBound1, tmpUpperBound2);

            int tmpLowerBound1 = floorDivision(_c - (sumMin - ithLowerBound), _as[i]);
            int tmpLowerBound2 = floorDivision(_c - (sumMax - ithUpperBound), _as[i]);
            tmpLowerBound = std::max(tmpLowerBound1, tmpLowerBound2);
        }

        sumMin += -ithLowerBound + tmpLowerBound;
        _bs[i]->removeBelow(tmpLowerBound);

        sumMax += -ithUpperBound + tmpUpperBound;
        _bs[i]->removeAbove(tmpUpperBound);
    }
    */


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
    int sumMin = 0;
    int sumMax = 0;
    for(size_t i = 0; i < _bs.size(); i += 1)
    {
        int ithBoundValue1 = _as[i] * _bs[i]->min();
        int ithBoundValue2 = _as[i] * _bs[i]->max();

        int ithLowerBound = std::min(ithBoundValue1, ithBoundValue2);
        int ithUpperBound = std::max(ithBoundValue1, ithBoundValue2);

        sumMin += ithLowerBound;
        sumMax += ithUpperBound;
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
    _r->propagateOnBoundChange(this);
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

void int_lin_ne::propagate(std::vector<int>& _as, std::vector<var<int>::Ptr>& _bs, int _c)
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
        _bs[0]->remove(_bs[0]->min());
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
        v->propagateOnBind(this);
    }
}
