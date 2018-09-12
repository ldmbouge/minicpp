#include "solver.hpp"
#include "cont.hpp"
#include "controller.hpp"
#include <assert.h>
#include <iostream>
#include <iomanip>

CPSolver::CPSolver()
    : _sm(new Trailer),
      _store(new Storage(_sm))
{
    _varId  = 0;
    _nbc = _nbf = _nbs = 0;
    _ctrl = nullptr;
}

CPSolver::~CPSolver()
{
   _iVars.clear();
   _store.dealloc();
   _sm.dealloc();
   std::cout << "CPSolver::~CPSolver(" << this << ")" << std::endl;
   Cont::shutdown();
}

void CPSolver::optimize(Objective::Ptr c)
{
   _objective = c;
}

void CPSolver::post(Constraint::Ptr c,bool enforceFixPoint)
{
    c->post();
    if (enforceFixPoint) 
        fixpoint();    
}

/**
 * Useful only for using continuation-style search combinator
 */
void CPSolver::branch(Constraint::Ptr c)
{
    try {
        c->post();
        fixpoint();
    } catch(Status s) {
        fail();
    }
}

void CPSolver::registerVar(AVar::Ptr avar)
{
   avar->setId(_varId++);
   _iVars.push_back(avar);
}

void CPSolver::notifyFixpoint()
{
   for(auto& body : _onFix)
      body();
}   

void CPSolver::fixpoint()
{
   try {
      notifyFixpoint();
      while (!_queue.empty()) {
         auto c = _queue.front();
         _queue.pop_front();
         c->setScheduled(false);
         if (c->isActive())
            c->propagate();
      }
      assert(_queue.size() == 0);
   } catch(Status x) {
      while (!_queue.empty()) {
         _queue.front()->setScheduled(false);
         _queue.pop_front();
      }
      //_queue.clear();
      assert(_queue.size() == 0);
      _nbf += 1;
      throw x;
   }
}

void CPSolver::solveOne(std::function<void(void)> b)
{
    Cont::initContinuationLibrary((int*)&b);
    _ctrl = new DFSController(_sm);
    _afterClose = _sm->push();
   Cont::Cont* k = Cont::Cont::takeContinuation();
   if (k->nbCalls()==0) {
      _ctrl->start(k);
      b();
   } else {
      std::cout<< "Done!" << std::endl;
   }
   delete _ctrl;
   _ctrl = nullptr;
   _sm->popToNode(_afterClose);
}

void CPSolver::tighten()
{
   if (_objective)
      _objective->tighten();
}

void CPSolver::solveAll(std::function<void(void)> b)
{
    Cont::initContinuationLibrary((int*)&b);
    _ctrl = new DFSController(_sm);
    _afterClose = _sm->push();
    Cont::Cont* k = Cont::Cont::takeContinuation();
   if (k->nbCalls()==0) {
      _ctrl->start(k);
      b();
      if (_objective)
         _objective->tighten();
      _ctrl->fail();
   } else {
      std::cout<< "Done!" << std::endl;
   }
   delete _ctrl;
   _ctrl = nullptr;
   _sm->popToNode(_afterClose);
}

void CPSolver::fail()
{
   if (_ctrl)
      _ctrl->fail();
}

// [LDM] This is for debugging purposes. Don't include when using valgrind

#if defined(__APPLE__)
void* operator new  ( std::size_t count )
{
   return malloc(count);
}
#endif
