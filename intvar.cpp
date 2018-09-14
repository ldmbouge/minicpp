#include "intvar.hpp"
#include "store.hpp"

IntVarImpl::IntVarImpl(CPSolver::Ptr& cps,int min,int max)
    : _solver(cps),
      _dom(new (cps) BitDomain(cps->getStateManager(),cps->getStore(),min,max)),  // allocate domain on stack allocator
      _onBindList(cps->getStateManager(),cps->getStore()),
      _onBoundsList(cps->getStateManager(),cps->getStore()),
      _onDomList(cps->getStateManager(),cps->getStore()),
      _domListener(new (cps) DomainListener(this))
{}

IntVarImpl::~IntVarImpl()
{
    std::cout << "IntVarImpl::~IntVarImpl called ?" << std::endl;
}

class ClosureConstraint : public Constraint {
    std::function<void(void)> _f;
public:
    ClosureConstraint(CPSolver::Ptr cp,std::function<void(void)>&& f)
        : Constraint(cp),
          _f(std::move(f)) {}
    void post() {}
    void propagate() {
        _f();
    }
};

revList<Constraint::Ptr>::revNode* IntVarImpl::whenBind(std::function<void(void)>&& f)
{
    return propagateOnBind(new (getSolver()) ClosureConstraint(_solver,std::move(f)));  
}
revList<Constraint::Ptr>::revNode* IntVarImpl::whenBoundsChange(std::function<void(void)>&& f)
{
    return propagateOnBoundChange(new (getSolver()) ClosureConstraint(_solver,std::move(f)));  
}
revList<Constraint::Ptr>::revNode* IntVarImpl::whenDomainChange(std::function<void(void)>&& f)
{
    return propagateOnDomainChange(new (getSolver()) ClosureConstraint(_solver,std::move(f)));  
}

void IntVarImpl::assign(int v)
{
    _dom->assign(v,*_domListener);
}
void IntVarImpl::remove(int v)
{
    _dom->remove(v,*_domListener);
}
void IntVarImpl::removeBelow(int newMin)
{
    _dom->removeBelow(newMin,*_domListener);
}
void IntVarImpl::removeAbove(int newMax)
{
    _dom->removeAbove(newMax,*_domListener);
}
void IntVarImpl::updateBounds(int newMin,int newMax)
{
    _dom->removeBelow(newMin,*_domListener);
    _dom->removeAbove(newMax,*_domListener);
}

void IntVarImpl::DomainListener::empty() 
{
    failNow();
}

void IntVarImpl::DomainListener::bind() 
{
    for(auto& f : theVar->_onBindList)
        theVar->_solver->schedule(f);
}

void IntVarImpl::DomainListener::change()  
{
    for(auto& f : theVar->_onDomList)
        theVar->_solver->schedule(f);
}

void IntVarImpl::DomainListener::changeMin() 
{
    for(auto& f :  theVar->_onBoundsList)
        theVar->_solver->schedule(f);
}

void IntVarImpl::DomainListener::changeMax() 
{
    for(auto& f :  theVar->_onBoundsList)
        theVar->_solver->schedule(f);
}

namespace Factory {
    var<int>::Ptr makeIntVar(CPSolver::Ptr cps,int min,int max) {
        var<int>::Ptr rv = new (cps) IntVarImpl(cps,min,max);  // allocate var on stack allocator
        cps->registerVar(rv);
        return rv;
    }
    var<int>::Ptr makeIntVar(CPSolver::Ptr cps,int n) {
        var<int>::Ptr rv = new (cps) IntVarImpl(cps,n);  // allocate var on stack allocator
        cps->registerVar(rv);
        return rv;
    }
    var<bool>::Ptr makeBoolVar(CPSolver::Ptr cps) {
        var<bool>::Ptr rv = new (cps) var<bool>(cps);
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

    std::vector<var<int>::Ptr,alloc> intVarArray(CPSolver::Ptr cps,int sz,int n)
    {
        std::vector<var<int>::Ptr,alloc> a(sz,(alloc(cps->getStore().get())));
        for(int i=0;i<sz;i++)
            a[i] = Factory::makeIntVar(cps,n);
        return a;
    }

    std::vector<var<int>::Ptr,alloc> intVarArray(CPSolver::Ptr cps,int sz) {
        std::vector<var<int>::Ptr,alloc> a(sz,(alloc(cps->getStore().get())));
        return a;
    }
};
