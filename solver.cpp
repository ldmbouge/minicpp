#include "solver.hpp"
#include "cont.hpp"
#include "controller.hpp"
#include <assert.h>
#include <iostream>
#include <iomanip>

CPSolver::CPSolver()
   : _ctx(new Engine),_cs(_ctx,Suspend)
{
    _closed = false;
    _varId  = 0;
    _nbc = _nbf = _nbs = 0;
    _ctrl = nullptr;
}

CPSolver::~CPSolver()
{
   for(auto& vp : _iVars)
      vp.dealloc();
   _iVars.clear();
   _iCstr.clear();
   Cont::shutdown();
}

Status CPSolver::add(Constraint::Ptr c)
{
    if (!_closed) {
        _iCstr.push_back(c);
        return Suspend;
    } else {
        try {
            c->post();
        } catch(Status x) {
            _queue.clear();
            return x;
        }
        Status s =  propagate();
	c = nullptr;
        if (s == Failure)
           fail();
        return s;
    }
}

void CPSolver::registerVar(AVar::Ptr avar)
{
   avar->setId(_varId++);
   _iVars.push_back(avar);
}

void CPSolver::close()
{
    for(auto c : _iCstr)
        c->post();
    _closed = true;
    propagate();
    _afterClose = _ctx->push();
    std::cout << "closed: " << _afterClose << std::endl;
}

Status CPSolver::propagate()
{
    try {
        while (!_queue.empty()) {
            auto cb = _queue.front();
            _queue.pop_front();
            cb();
        }
        assert(_queue.size() == 0);
        return _cs = Suspend;
    } catch(Status x) {
        _queue.clear();
        assert(_queue.size() == 0);
        _nbf += 1;
        return _cs = Failure;
    }
}

void CPSolver::solveOne(std::function<void(void)> b)
{
   Cont::initContinuationLibrary((int*)&b);
   _ctrl = new DFSController(_ctx);
   Cont::Cont* k = Cont::Cont::takeContinuation();
   if (k->nbCalls()==0) {
      _ctrl->start(k);
      b();
      //_ctrl->fail();
   } else {
      std::cout<< "Done!" << std::endl;
   }
   delete _ctrl;
   _ctrl = nullptr;
   //_ctx->popToNode(_afterClose);
}

void CPSolver::solveAll(std::function<void(void)> b)
{
   Cont::initContinuationLibrary((int*)&b);
   _ctrl = new DFSController(_ctx);
   Cont::Cont* k = Cont::Cont::takeContinuation();
   if (k->nbCalls()==0) {
      _ctrl->start(k);
      b();
      _ctrl->fail();
   } else {
      std::cout<< "Done!" << std::endl;
   }
   delete _ctrl;
   _ctrl = nullptr;
   _ctx->popToNode(_afterClose);
}

void CPSolver::fail()
{
   if (_ctrl)
      _ctrl->fail();
}


// [LDM] This is for debugging purposes. Don't include when using valgrind
/*
void* operator new  ( std::size_t count )
{
   return malloc(count);
}
*/
