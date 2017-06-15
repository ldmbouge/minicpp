#include "intvar.hpp"

var<int>::var(CPSolver::Ptr& cps,int min,int max)
   : _solver(cps),
     _dom(new (cps) BitDomain(cps->engine(),min,max)),  // allocate domain on stack allocator
     _onBindList(cps->engine()),
     _onBoundsList(cps->engine())
{}

var<int>::~var<int>()
{
   std::cout << "var<int>::~var<int> called ?" << std::endl;
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
      var<int>::Ptr rv = new (cps) var<int>(cps,min,max);  // allocate var on stack allocator
      cps->registerVar(rv);
      return rv;
   }
   /**
    * ldm : This factory method for a vector of var<int> is meant to not only allocate the vector
    *       and the elements, but, more importantly, to allocate on the library Store (Storage type).
    *       To Make STL work peacefully here, we need an _adapter_ type that wraps our own allocator
    *       and passes it to the vector constructor. Note that this is a stateful allocator and its
    *       type is also part of the STL vector template. So this is no longer a vector<var<int>::Ptr>
    *       Rather it is now a vector<var<int>::Ptr,alloc> where alloc is the type of the allocator
    *       adapter (see the .hpp file for its definition!). STL allocators are _typed_ for
    *       what type of value they can allocate. We would need one using clause per type of allocator
    *       we might ever need (ugly, but that's STL's constraint). 
    *       With the 'auto' keyword, this is invisible to the modeler. Clearly, vector<var<int>::Ptr> and
    *       vector<var<int>::Ptr,alloc> are two distinct and incompatible types. Either use auto, or 
    *       rely on C++ decltype primitive. 
    *       The mechanism for allocating STL objects fully on the system stack is the same. Only caveat
    *       You first need to create an stl::CoreAlloc (a class of mine) to grab space on the stack and
    *       create an adapter from it again. Observe how CoreAlloc and Storage both have the same API. They
    *       are both stack-allocator (meaning FIFO allocation, no free). 
    */
   std::vector<var<int>::Ptr,alloc> intVarArray(CPSolver::Ptr cps,int sz,int min,int max) {
      std::vector<var<int>::Ptr,alloc> a(sz,(alloc(cps->getStore().get())));
      for(int i=0;i<sz;i++)
         a[i] = Factory::makeIntVar(cps,min,max);
      return a;
   }
};
