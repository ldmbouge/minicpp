#ifndef __EXPLAINER_H
#define __EXPLAINER_H

#include "solver.hpp"
#include "handle.hpp"
#include "intvar.hpp"
#include "domain.hpp"
#include "literal.hpp"
#include "hashtable.hpp"
#include "trail.hpp"
#include "expTrail.hpp"
#include "fail.hpp"
#include "visitor.hpp"
#include "globalCutQueue.hpp"
#include <vector>
#include <unordered_map>

class ExpListener;
class ExpSolver;
typedef TrailHashtable<unsigned long, Literal> LitHashTable;

class Explainer {
    ExpSolver* _es;
    ExpTrailer::Ptr _expT;
    GlobalCutQueue _cutQueue;
    std::vector<ExpListener*> _listeners;
    std::unordered_map<unsigned long, Literal*> _nogood;
    int _failDepth;
    // Pool::Ptr    _pool;
public:
    typedef handle_ptr<Explainer> Ptr;
    Explainer(ExpSolver* es);
    void injectListeners();
    void setFailDepth(int);
    void empty(var<int>::Ptr, FailExpl, int, FailExpl, int);
    void bind(var<int>::Ptr, int);
    void remove(var<int>::Ptr, int);
    void changeMin(var<int>::Ptr, int);
    void changeMax(var<int>::Ptr, int);
    void setTrailer(ExpTrailer::Ptr ep) { _expT = ep;}
    void setNoGood(std::vector<Literal*>);
    int getCurrDepth();
    void clearNoGood();
    void printNoGood();
    void addCut();
    void checkCuts();
    void checkLit(Literal* lp);
    Literal* findLit(Literal&);
};

template<> class handle_ptr<ExpSolver>;
class SearchStatistics;

class ExpSolver {
    CPSolver::Ptr _cps;
    handle_ptr<Explainer> _exp;
    SearchStatistics* _ss;
public:
    typedef handle_ptr<ExpSolver> Ptr;
    friend class Explainer;
    ExpSolver();
    ~ExpSolver();
    Status status() { return _cps->status();}
    Trailer::Ptr getStateManager() { return _cps->getStateManager();}
    Storage::Ptr getStore() { return _cps->getStore();}
    CPSolver::Ptr getSolver() { return _cps;}
    int getChoices() const { return _cps->_nbc;}
    int getFailures() const { return _cps->_nbf;}
    int getSolutions() const { return _cps->_nbs;}
    handle_ptr<Explainer> getExplainer() { return _exp;}
    handle_ptr<Constraint> getCurrConstraint() { return _cps->_currCon;}
    void setSearchStats(SearchStatistics* stats) { _ss = stats;}
    int getDepth();
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
    var<int>::Ptr _x;
    IntNotifier* _notif;
public:
    typedef handle_ptr<ExpListener> Ptr;
    ExpListener(Explainer*, var<int>::Ptr);
    void empty() override;
    void empty(FailExpl, int, FailExpl, int) override;
    void change() override;
    void bind(int) override;
    void changeMin(int) override;
    void changeMax(int) override;
    void remove(int) override;
};

// class AllDifferentAC;
// class AllDiffExplainer {
//     ExpSolver::Ptr _es;
//     Explainer::Ptr _exp;
//     AllDifferentAC* _c;
// public:
//     typedef handle_ptr<AllDiffExplainer> Ptr;
//     AllDiffExplainer(ExpSolver* es, AllDifferentAC* c);
//     ~AllDiffExplainer() {}
//     std::vector<Literal*> explain(Literal* lp) { return std::vector<Literal*>({lp});}
//     void explain(var<int>::Ptr x, int val);
// };


// class EQc;
// class EQcExplainer {
//     ExpSolver::Ptr _es;
//     Explainer::Ptr _exp;
//     EQc* _c;
// public:
//     EQcExplainer(ExpSolver* es, EQc* c)
//       : _es(es), _c(c) {}
//     ~EQcExplainer() {}
//     std::vector<Literal*> explain(Literal* lp);
// };

namespace Factory {
   inline ExpSolver::Ptr makeExpSolver() { return new ExpSolver;}
};

#endif
