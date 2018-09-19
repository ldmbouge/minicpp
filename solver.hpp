#ifndef __SOLVER_H
#define __SOLVER_H

#include <list>
#include <deque>
#include <functional>
#include <stdlib.h>

#include "cont.hpp"
#include "handle.hpp"
#include "fail.hpp"
#include "store.hpp"
#include "avar.hpp"
#include "acstr.hpp"
#include "controller.hpp"
#include "trailable.hpp"

typedef std::reference_wrapper<std::function<void(void)>> Closure;
class Controller;

class CPSolver {
    Trailer::Ptr                  _sm;
    Storage::Ptr               _store;
    std::list<AVar::Ptr>       _iVars;
    std::deque<Constraint::Ptr>   _queue;
    std::list<std::function<void(void)>>  _onFix;
    long                  _afterClose;
    int                        _varId;
    int                          _nbc; // # choices
    int                          _nbf; // # fails
    int                          _nbs; // # solutions
    Controller*                 _ctrl;
public:
    template<typename T> friend class var;
    typedef handle_ptr<CPSolver> Ptr;
    CPSolver();
    ~CPSolver();
    Trailer::Ptr getStateManager()       { return _sm;}
    Storage::Ptr getStore()              { return _store;}
    void registerVar(AVar::Ptr avar);
    void schedule(Constraint::Ptr& c) {
        if (c->isActive() && !c->isScheduled()) {
            c->setScheduled(true);
            _queue.emplace_back(c);
        }
    }
    void onFixpoint(std::function<void(void)>& cb) { _onFix.emplace_back(cb);}
    void notifyFixpoint();
    void tighten();
    void fixpoint();
    void post(Constraint::Ptr c,bool enforceFixPoint=true);
    void branch(Constraint::Ptr c);
    void incrNbChoices() { _nbc += 1;}
    void incrNbSol()     { _nbs += 1;}
    void solveOne(std::function<void(void)> b);
    void solveAll(std::function<void(void)> b);
    template <class Body1,class Body2> void tryBin(Body1 left,Body2 right);
    void fail();
    friend void* operator new(std::size_t sz,CPSolver::Ptr e);
    friend void* operator new[](std::size_t sz,CPSolver::Ptr e);
    friend std::ostream& operator<<(std::ostream& os,const CPSolver& s) {
        return os << "CPSolver(" << &s << ")" << std::endl
                  << "\t#choices   = " << s._nbc << std::endl
                  << "\t#fail      = " << s._nbf << std::endl
                  << "\t#solutions = " << s._nbs << std::endl;
    }
};

namespace Factory {
   inline CPSolver::Ptr makeSolver() { return new CPSolver;}
};

inline void* operator new(std::size_t sz,CPSolver::Ptr e)
{
   return e->_store->allocate(sz);
}

inline void* operator new[](std::size_t sz,CPSolver::Ptr e)
{
   return e->_store->allocate(sz);
}

//void* operator new  ( std::size_t count );

template <class Container,class FIt,class Body>
void withVarDo(Container& c,FIt it,Body b)
{
   if (it == c.end())
      return;
   auto& x = *it;
   b(x);
}

template <class Body1,class Body2>
void CPSolver::tryBin(Body1 left,Body2 right) 
{
   Cont::Cont* k = Cont::Cont::takeContinuation();
   if (k->nbCalls()==0) {
      _nbc++;
      _ctrl->addChoice(k);
      left();
   } else {
      Cont::letgo(k);
      _nbc++;
      _ctrl->trust();
      right();
   }
}

#endif
