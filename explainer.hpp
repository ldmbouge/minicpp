#ifndef __EXPLAINER_H
#define __EXPLAINER_H

#include "solver.hpp"
#include "handle.hpp"
#include "handle.hpp"
#include "intvar.hpp"
#include "domain.hpp"
// #include "constraint.hpp"


class ExpListener;
class ExpSolver;
class Explainer {
    ExpSolver* _es;
    std::vector<ExpListener*> _listeners;  // should be a separate stack-like object
public:
    typedef handle_ptr<Explainer> Ptr;
    Explainer(ExpSolver* es) : _es(es), _listeners(0) {}
    void injectListeners();
    void bind(AVar::Ptr, int);
    void remove(AVar::Ptr, int);
    void changeMin(AVar::Ptr, int);
    void changeMax(AVar::Ptr, int);
};

template<> class handle_ptr<ExpSolver>;
class ExpSolver {
    CPSolver::Ptr _cps;
    handle_ptr<Explainer> _exp;
public:
    typedef handle_ptr<ExpSolver> Ptr;
    friend class Explainer;
    ExpSolver() : _cps(new CPSolver), _exp(new Explainer(this)) {}
    Trailer::Ptr getStateManager() { return _cps->getStateManager();}
    Storage::Ptr getStore() { return _cps->getStore();}
    CPSolver::Ptr getSolver() { return _cps;}
    int getChoices() const { return _cps->_nbc;}
    int getFailures() const { return _cps->_nbf;}
    int getSolutions() const { return _cps->_nbs;}
    handle_ptr<Explainer> getExplainer() { return _exp;}
    // void injectListeners() { _exp->injectListeners();}
    void registerVar(AVar::Ptr avar) { _cps->registerVar(avar);}
    void schedule(Constraint::Ptr& c) { _cps->schedule(c);}
    void onFixpoint(std::function<void(void)>& cb) { _cps->onFixpoint(cb);}
    void notifyFixpoint() { _cps->notifyFixpoint();}
    void tighten() { _cps->tighten();}
    void fixpoint() { _cps->fixpoint();}
    void fixpointNT() { _cps->fixpointNT();}
    void post(Constraint::Ptr c,bool enforceFixPoint=true) { _cps->post(c, enforceFixPoint);}
    void incrNbChoices() { _cps->incrNbChoices();}
    void incrNbSol() { _cps->incrNbSol();}
    void fail() { _cps->fail();}
    friend void* operator new(std::size_t sz,ExpSolver::Ptr e);
    friend void* operator new[](std::size_t sz,ExpSolver::Ptr e);
    friend std::ostream& operator<<(std::ostream& os,const ExpSolver& s) {
        return os << "ExpSolver(" << &s << ")" << std::endl
                  << "\t#choices   = " << s.getChoices() << std::endl
                  << "\t#fail      = " << s.getFailures() << std::endl
                  << "\t#solutions = " << s.getSolutions() << std::endl;
    }
};



template <>
class handle_ptr<ExpSolver> {
   ExpSolver* _ptr;
public:
   template <typename DT> friend class handle_ptr;
   typedef ExpSolver element_type;
   typedef ExpSolver* pointer;
   operator handle_ptr<CPSolver>() { return _ptr->getSolver();}
   handle_ptr() noexcept : _ptr(nullptr) {}
   handle_ptr(ExpSolver* ptr) noexcept : _ptr(ptr) {}
   handle_ptr(const handle_ptr<ExpSolver>& ptr) noexcept : _ptr(ptr._ptr)  {}
   handle_ptr(std::nullptr_t ptr)  noexcept : _ptr(ptr) {}
   handle_ptr(handle_ptr<ExpSolver>&& ptr) noexcept : _ptr(std::move(ptr._ptr)) {}
   template <typename DT> handle_ptr(DT* ptr) noexcept : _ptr(ptr) {}
   template <typename DT> handle_ptr(const handle_ptr<DT>& ptr) noexcept : _ptr(ptr.get()) {}   
   template <typename DT> handle_ptr(handle_ptr<DT>&& ptr) noexcept : _ptr(std::move(ptr._ptr)) {}
   handle_ptr& operator=(const handle_ptr<ExpSolver>& ptr) { _ptr = ptr._ptr;return *this;}
   handle_ptr& operator=(handle_ptr<ExpSolver>&& ptr)      { _ptr = std::move(ptr._ptr);return *this;}
   handle_ptr& operator=(ExpSolver* ptr)                   { _ptr = ptr;return *this;}
   element_type* get() const noexcept { return _ptr;}
   element_type* operator->() const noexcept { return _ptr;}
   element_type& operator*() const noexcept  { return *_ptr;}
   template<typename SET> SET& operator*() const noexcept {return *_ptr;}
   operator bool() const noexcept { return _ptr != nullptr;}
   void dealloc() { delete _ptr;_ptr = nullptr;}
   void free()    { delete _ptr;_ptr = nullptr;}
};

inline void* operator new(std::size_t sz,ExpSolver::Ptr e)
{
   return e->getStore()->allocate(sz);
}

inline void* operator new[](std::size_t sz,ExpSolver::Ptr e)
{
   return e->getStore()->allocate(sz);
}

class ExpListener : public IntNotifier {
    Explainer* _exp;
    AVar::Ptr _x;
    IntNotifier* _notif;
public:
    typedef handle_ptr<ExpListener> Ptr;
    ExpListener(Explainer* exp, AVar::Ptr x) : _exp(exp), _x(x), _notif(x->getListener()) { _x->setListener(this);}
    void empty() override { _notif->empty();}
    void change() override { _notif->change();}
    void bind(int a) override { _exp->bind(_x, a); _notif->bind(a);}
    void changeMin(int newMin) override { _exp->changeMin(_x, newMin); _notif->changeMin(newMin);}
    void changeMax(int newMax) override { _exp->changeMax(_x, newMax); _notif->changeMax(newMax);}
    void remove(int a) override { _exp->remove(_x, a);}
};

class AllDifferentAC;
class AllDiffExplainer {
    AllDifferentAC* _c;
public:
    typedef handle_ptr<AllDiffExplainer> Ptr;
    AllDiffExplainer(AllDifferentAC* c)
        : _c(c) {}
    ~AllDiffExplainer() {}
    void explain(Literal& l) {};
    void explain(var<int>::Ptr x, int val);
};

namespace Factory {
   inline ExpSolver::Ptr makeExpSolver() { return new ExpSolver;}
};

#endif
