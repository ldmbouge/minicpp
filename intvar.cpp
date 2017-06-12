#include "intvar.hpp"

var<int>::var(CPSolver::Ptr& cps,int min,int max)
    : _solver(cps),
      _dom(std::make_unique<BitDomain>(cps->context(),min,max)),
      _onBindList(cps->context()),
      _onBoundsList(cps->context())
{}

var<int>::~var<int>()
{
   std::cout << "dealloc(" << this << ')' << std::endl;
   std::cout << "\tonBIND  :" << _onBindList << std::endl;
   std::cout << "\tonBOUNDS:" << _onBoundsList << std::endl;
}

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
    for(auto& f :  _onBindList)
       _solver->schedule(f);
    for(auto& f :  _onBoundsList)
       _solver->schedule(f);
}

void var<int>::domEvt(int sz)  
{
   if (sz==1)
      for(auto& f : _onBindList)
         _solver->schedule(f);
}

void var<int>::updateMinEvt(int sz) 
{
    for(auto& f :  _onBoundsList)
        _solver->schedule(f);
    if (sz==1)
        for(auto& f : _onBindList)
            _solver->schedule(f);
}

void var<int>::updateMaxEvt(int sz) 
{
   for(auto& f :  _onBoundsList)
      _solver->schedule(f);
   if (sz==1)
      for(auto& f : _onBindList)
         _solver->schedule(f);
}

namespace Factory {
    var<int>::Ptr makeIntVar(CPSolver::Ptr cps,int min,int max) {
        var<int>::Ptr rv = new var<int>(cps,min,max);
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
