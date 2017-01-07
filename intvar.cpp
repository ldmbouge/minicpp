#include "intvar.hpp"

var<int>::var(CPSolver::Ptr& cps,int min,int max)
    : _solver(cps),
      _dom(std::make_unique<BitDomain>(cps->_ctx,min,max))
{}

void var<int>::bind(int v)
{
    _dom->bind(v,*this);
}
void var<int>::remove(int v)
{
    _dom->remove(v,*this);
}
void var<int>::updateMin(int newMin)
{
    _dom->updateMin(newMin,*this);
}
void var<int>::updateMax(int newMax)
{
    _dom->updateMax(newMax,*this);
}
void var<int>::updateBounds(int newMin,int newMax)
{
    _dom->updateMin(newMin,*this);
    _dom->updateMax(newMax,*this);
}

void var<int>::bindEvt() 
{
    CPSolver::Ptr solver = _solver.lock();
    for(auto& f :  _onBindList)
        solver->schedule(f);
    for(auto& f :  _onBoundsList)
        solver->schedule(f);
}

void var<int>::domEvt(int sz)  
{
    CPSolver::Ptr solver = _solver.lock();
    if (sz==1)
        for(auto& f : _onBindList)
            solver->schedule(f);
}

void var<int>::updateMinEvt(int sz) 
{
    CPSolver::Ptr solver = _solver.lock();
    for(auto& f :  _onBoundsList)
        solver->schedule(f);
    if (sz==1)
        for(auto& f : _onBindList)
            solver->schedule(f);
}

void var<int>::updateMaxEvt(int sz) 
{
    CPSolver::Ptr solver = _solver.lock();
    for(auto& f :  _onBoundsList)
        solver->schedule(f);
    if (sz==1)
        for(auto& f : _onBindList)
            solver->schedule(f);
}

namespace Factory {
    var<int>::Ptr makeIntVar(CPSolver::Ptr cps,int min,int max) {
        var<int>::Ptr rv = std::make_shared<var<int>>(cps,min,max);
        cps->registerVar(rv);
        return rv;
    }
    std::vector<var<int>::Ptr> intVarArray(CPSolver::Ptr cps,int sz,int min,int max) {
        std::vector<var<int>::Ptr> a(sz);
        for(int i=0;i<sz;i++)
            a[i] = Factory::makeIntVar(cps,min,max);
        return a;
    }
};
