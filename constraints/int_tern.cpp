#include <algorithm>
#include <cmath>
#include <limits>
#include <constraints/int_tern.hpp>

int_tern::int_tern(CPSolver::Ptr cp, std::vector<var<int>::Ptr>* intVars, std::vector<var<bool>::Ptr>* boolVars, std::vector<int> const& vars, std::vector<int> const& consts) :
    Constraint(cp),
    _a(intVars->at(vars[0])),
    _b((intVars->at(vars[1]))),
    _c((intVars->at(vars[2])))
{}


void int_tern::calMulMinMax(int aMin, int aMax, int bMin, int bMax, int& min, int& max)
{
    int bounds[4];
    bounds[0] = aMin * bMin;
    bounds[1] = aMax * bMin;
    bounds[2] = aMin * bMax;
    bounds[3] = aMax * bMax;

    min = std::numeric_limits<int>::max();
    max = std::numeric_limits<int>::min();
    for(int i = 0; i < 4; i += 1)
    {
        min = std::min(min, bounds[i]);
        max = std::max(max, bounds[i]);
    }
}

void int_tern::calDivMinMax(int aMin, int aMax, int bMin, int bMax, int& min, int& max)
{
    double bounds[4];
    int boundsCount = 0;
    if(bMin != 0)
    {
        bounds[0] = aMin / bMin;
        bounds[1] = aMax / bMin;
        boundsCount += 2;
    }
    if(bMax != 0)
    {
        bounds[0] = aMin / bMax;
        bounds[1] = aMax / bMax;
        boundsCount += 2;
    }

    min = std::numeric_limits<int>::max();
    max = std::numeric_limits<int>::min();
    for(int i = 0; i < boundsCount; i += 1)
    {
        min = std::min(min, static_cast<int>(ceil(bounds[i])));
        max = std::max(max, static_cast<int>(floor(bounds[i])));
    }
}

void int_tern::post()
{
    _a->propagateOnBoundChange(this);
    _b->propagateOnBoundChange(this);
    _c->propagateOnBoundChange(this);
}

int_div::int_div(CPSolver::Ptr cp, std::vector<var<int>::Ptr> *intVars, std::vector<var<bool>::Ptr> *boolVars, const std::vector<int> &vars, const std::vector<int> &consts) :
    int_tern(cp, intVars, boolVars, vars, consts)
{}

void int_div::post()
{
    _b->remove(0);
    int_tern::post();
    propagate();
}

void int_div::propagate()
{
    //Semantic: a / b = c
    int aMin = _a->min();
    int aMax = _a->max();
    int bMin = _b->min();
    int bMax = _b->max();
    int cMin = _c->min();
    int cMax = _c->max();
    int boundsMin;
    int boundsMax;

    //Propagation: a / b -> c
    calDivMinMax(aMin, aMax, bMin, bMax, boundsMin, boundsMax);
    _c->updateBounds(boundsMin, boundsMax);

    //Propagation: a / b <- c
    calMulMinMax(cMin, cMax, bMin, bMax, boundsMin, boundsMax);
    _a->updateBounds(boundsMin, boundsMax);
    calDivMinMax(aMin, aMax, cMin, cMax, boundsMin, boundsMax);
    _b->updateBounds(boundsMin, boundsMax);
}

int_max::int_max(CPSolver::Ptr cp, std::vector<var<int>::Ptr> *intVars, std::vector<var<bool>::Ptr> *boolVars, const std::vector<int> &vars, const std::vector<int> &consts) :
    int_tern(cp, intVars, boolVars, vars, consts)
{}

void int_max::post()
{
    int_tern::post();
    propagate();
}

void int_max::propagate()
{
    //Semantic: max(a,b) = c
    int aMin = _a->min();
    int aMax = _a->max();
    int bMin = _b->min();
    int bMax = _b->max();
    int cMax = _c->max();

    //Propagation: max(a,b) -> c
    _c->updateBounds(std::max(aMin, bMin), std::max(aMax, bMax));

    //Propagation: max(a,b) <- c
    _a->removeAbove(cMax);
    _b->removeAbove(cMax);
}

int_min::int_min(CPSolver::Ptr cp, std::vector<var<int>::Ptr> *intVars, std::vector<var<bool>::Ptr> *boolVars, const std::vector<int> &vars, const std::vector<int> &consts) :
    int_tern(cp, intVars, boolVars, vars, consts)
{}

void int_min::post()
{
    int_tern::post();
    propagate();
}

void int_min::propagate()
{
    //Semantic: min(a,b) = c
    int aMin = _a->min();
    int aMax = _a->max();
    int bMin = _b->min();
    int bMax = _b->max();
    int cMin = _c->min();

    //Propagation: min(a,b) -> c
    _c->updateBounds(std::min(aMin, bMin), std::min(aMax, bMax));

    //Propagation: min(a,b) <- c
    _a->removeBelow(cMin);
    _b->removeBelow(cMin);
}

int_mod::int_mod(CPSolver::Ptr cp, std::vector<var<int>::Ptr> *intVars, std::vector<var<bool>::Ptr> *boolVars, const std::vector<int> &vars, const std::vector<int> &consts) :
    int_tern(cp, intVars, boolVars, vars, consts)
{}

void int_mod::post()
{
    int_tern::post();
    propagate();
}

void int_mod::propagate()
{
    // TODO
    assert(false);
    exit(EXIT_FAILURE);

}

int_plus::int_plus(CPSolver::Ptr cp, std::vector<var<int>::Ptr> *intVars, std::vector<var<bool>::Ptr> *boolVars, const std::vector<int> &vars, const std::vector<int> &consts) :
    int_tern(cp, intVars, boolVars, vars, consts)
{}

void int_plus::post()
{
    int_tern::post();
    propagate();
}

void int_plus::propagate()
{
    //Semantic: a + b = c
    int aMin = _a->min();
    int aMax = _a->max();
    int bMin = _b->min();
    int bMax = _b->max();
    int cMin = _c->min();
    int cMax = _c->max();
    int boundsMin;
    int boundsMax;

    //Propagation: a + b -> c
    boundsMin = aMin + bMin;
    boundsMax = aMax + bMax;
    _c->updateBounds(boundsMin, boundsMax);

    //Propagation: a + b <- c
    boundsMin = cMin - bMax;
    boundsMax = cMax - bMin;
    _a->updateBounds(boundsMin, boundsMax);
    boundsMin = cMin - aMax;
    boundsMax = cMax - aMin;
    _b->updateBounds(boundsMin, boundsMax);
}

int_pow::int_pow(CPSolver::Ptr cp, std::vector<var<int>::Ptr> *intVars, std::vector<var<bool>::Ptr> *boolVars, const std::vector<int> &vars, const std::vector<int> &consts) :
    int_tern(cp, intVars, boolVars, vars, consts)
{}

void int_pow::post()
{
    int_tern::post();
    propagate();
}

void int_pow::propagate()
{
    // TODO
    assert(false);
    exit(EXIT_FAILURE);
}

int_times::int_times(CPSolver::Ptr cp, std::vector<var<int>::Ptr> *intVars, std::vector<var<bool>::Ptr> *boolVars, const std::vector<int> &vars, const std::vector<int> &consts) :
    int_tern(cp, intVars, boolVars, vars, consts)
{}

void int_times::post()
{
    int_tern::post();
    propagate();
}

void int_times::propagate()
{
    //Semantic: a * b = c
    int aMin = _a->min();
    int aMax = _a->max();
    int bMin = _b->min();
    int bMax = _b->max();
    int cMin = _c->min();
    int cMax = _c->max();
    int boundsMin;
    int boundsMax;

    //Propagation: a * b -> c
    calMulMinMax(aMin, aMax, bMin, bMax, boundsMin, boundsMax);
    _c->updateBounds(boundsMin, boundsMax);

    //Propagation: a * b <- c
    calDivMinMax(cMin, cMax, bMin, bMax, boundsMin, boundsMax);
    _a->updateBounds(boundsMin, boundsMax);
    calDivMinMax(cMin, cMax, aMin, aMax, boundsMin, boundsMax);
    _b->updateBounds(boundsMin, boundsMax);
}


